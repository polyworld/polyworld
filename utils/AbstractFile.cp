#include "AbstractFile.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define GZIP_EXT ".gz"

AbstractFile *AbstractFile::open( ConcreteFileType type,
								  const char *abstractPath,
								  const char *mode )
{
	return new AbstractFile( type, abstractPath, mode );
}

AbstractFile *AbstractFile::open( const char *abstractPath,
								  const char *mode )
{
	return new AbstractFile( abstractPath, mode );
}

bool AbstractFile::exists(  const char *abstractPath,
							bool *isAmbiguous,
							ConcreteFileType *typeFound )
{
	int numFound = 0;
	ConcreteFileType type;

	if( exists(TYPE_FILE, abstractPath) )
	{
		numFound++;
		type = TYPE_FILE;
	}

	if( exists(TYPE_GZIP_FILE, abstractPath) )
	{
		numFound++;
		type = TYPE_GZIP_FILE;
	}

	if( isAmbiguous != NULL )
	{
		*isAmbiguous = numFound > 1;
		if( *isAmbiguous )
		{
			type = TYPE_UNDEFINED;
		}
	}

	if( typeFound != NULL )
	{
		*typeFound = type;
	}

	return numFound > 0;
}

bool AbstractFile::exists( ConcreteFileType type, const char *abstractPath )
{
	bool retval = false;
	char *path = createPath( type, abstractPath );

	struct stat buf;
	retval = stat( path, &buf ) == 0;
	free( path );

	return retval;
}

int AbstractFile::link( const char *oldAbstractPath,
						const char *newAbstractPath )
{
	int rc = -1;

	if( !exists(newAbstractPath) )
	{
		bool isAmbiguous;
		ConcreteFileType type;
		bool found;

		found = exists( oldAbstractPath,
						&isAmbiguous,
						&type );
		if( found && !isAmbiguous )
		{
			char *oldpath = createPath( type, oldAbstractPath );
			char *newpath = createPath( type, newAbstractPath );

			rc = ::link( oldpath, newpath );

			free( oldpath );
			free( newpath );
		}
	}

	return rc;
}

int AbstractFile::unlink( const char *abstractPath )
{
	int rc = -1;

	bool isAmbiguous;
	ConcreteFileType type;
	bool found;

	found = exists( abstractPath,
					&isAmbiguous,
					&type );
	if( found && !isAmbiguous )
	{
		char *path = createPath( type, abstractPath );

		rc = ::unlink( path );

		free( path );
	}

	return rc;
}

int AbstractFile::rename( const char *oldAbstractPath,
						  const char *newAbstractPath )
{
	int rc = -1;

	if( !exists(newAbstractPath) )
	{
		bool isAmbiguous;
		ConcreteFileType type;
		bool found;

		found = exists( oldAbstractPath,
						&isAmbiguous,
						&type );
		if( found && !isAmbiguous )
		{
			char *oldpath = createPath( type, oldAbstractPath );
			char *newpath = createPath( type, newAbstractPath );

			rc = ::rename( oldpath, newpath );

			free( oldpath );
			free( newpath );
		}
	}

	return rc;
}

AbstractFile::AbstractFile( ConcreteFileType type,
							const char *abstractPath,
							const char *mode )
{
	init( type, abstractPath, mode );
}

AbstractFile::AbstractFile( const char *abstractPath,
							const char *mode )
{
	ConcreteFileType type;
	int numFound = 0;

	if( exists(TYPE_GZIP_FILE, abstractPath) )
	{
		type = TYPE_GZIP_FILE;
		numFound++;
	}

	if( exists(TYPE_FILE, abstractPath) )
	{
		type = TYPE_FILE;
		numFound++;
	}

	if( numFound > 1 )
	{
		if( &abstractPath[strlen(abstractPath)-strlen(GZIP_EXT)] )	// specified path ends in .gz
			type = TYPE_GZIP_FILE;
		else
			type = TYPE_FILE;
	}

	if( numFound < 1 )
	{
		fprintf( stderr, "Failed opening %s \n", abstractPath );
		exit( 1 );
	}

	init( type, abstractPath, mode );
}

AbstractFile::~AbstractFile()
{
	close();
}

int AbstractFile::close()
{
	int rc = 0;

	switch( type )
	{
	case TYPE_FILE:
		if( file.fp )
		{
			rc = fclose( file.fp );
			file.fp = NULL;
		}
		break;
	case TYPE_GZIP_FILE:
		if( gzip.path )
		{
			free( (void *)gzip.path );
			gzip.path = NULL;
		}
		if( gzip.fp )
		{
			rc = gzclose( gzip.fp );
			gzip.fp = NULL;
		}
		break;
	default:
		assert( false );
	}

	if( abstractPath )
	{
		free( (void *)abstractPath );
		abstractPath = NULL;
	}

	return rc;
}

const char *AbstractFile::getAbstractPath()
{
	return abstractPath;
}

bool AbstractFile::isCapable( ConcreteFileCapability cap )
{
	bool retval;

	switch( type )
	{
	case TYPE_FILE:
		{
			// we can do it all.
			retval = true;
		}
		break;
	case TYPE_GZIP_FILE:
		{
			switch( cap )
			{
			case CAP_SEEK_END:
			case CAP_REWRITE:
				retval = false;
				break;
			default:
				retval = true;
			}
		}
		break;
	default:
		assert( false );
	}
	
	return retval;
}


size_t AbstractFile::write( const void *ptr, size_t size, size_t nmemb )
{
	size_t rc = 0;

	switch( type )
	{
	case TYPE_FILE:
		{
			rc = fwrite( ptr, size, nmemb, file.fp );
		}
		break;
	case TYPE_GZIP_FILE:
		{
			int rc_gzwrite = gzwrite( gzip.fp, ptr, size * nmemb );
			if( rc_gzwrite > 0 )
			{
				rc = rc_gzwrite / size;
			}
		}
		break;
	default:
		assert( false );
	}
	
	return rc;
}

int AbstractFile::printf( const char *format, ... )
{
	const int BUFLEN = 1024 * 8;

	int rc = 0;
	va_list argp;
	char stackbuf[BUFLEN];

	va_start( argp, format );

	rc = vsnprintf( stackbuf, BUFLEN, format, argp );
	assert( rc < BUFLEN );

	va_end( argp );

	size_t rc_write = write( stackbuf, 1, rc );
	if( rc_write != (size_t)rc )
	{
		rc = -1;
	}

	return rc;
}

int AbstractFile::flush( bool full )
{
	int rc = 0;

	switch( type )
	{
	case TYPE_FILE:
		{
			rc = fflush( file.fp );
		}
		break;
	case TYPE_GZIP_FILE:
		{
			// Flushing degrades compression, so try to avoid it.
			if( full )
			{
				rc = gzflush( gzip.fp, Z_SYNC_FLUSH );
			}
		}
		break;
	default:
		assert( false );
	}

	return rc;
}

size_t AbstractFile::read( void *ptr, size_t size, size_t nmemb )
{
	size_t rc = 0;

	switch( type )
	{
	case TYPE_FILE:
		{
			rc = fread( ptr, size, nmemb, file.fp );
		}
		break;
	case TYPE_GZIP_FILE:
		{
			int rc_gzread = gzread( gzip.fp, ptr, size * nmemb );
			if( rc_gzread > 0 )
			{
				rc = rc_gzread / size;
			}
		}
		break;
	default:
		assert( false );
	}

	return rc;
}

// WARNING! This will only work on a single line at a time, where
// the line must be shorter than 4K bytes.
int AbstractFile::scanf( const char *format, ... )
{
	int rc = 0;

	const int BUFLEN = 1024 * 4;
	char buf[BUFLEN];
	char *line;

	line = this->gets( buf, BUFLEN );
	if( line == NULL )
	{
		rc = -1;
	}
	else
	{
		va_list argp;
		va_start( argp, format );

		rc = vsscanf( line, format, argp );

		va_end( argp );
	}

	return rc;
}

char *AbstractFile::gets( char *s, int size )
{
	char *retval = 0;

	switch( type )
	{
	case TYPE_FILE:
		{
			retval = fgets( s, size, file.fp );
		}
		break;
	case TYPE_GZIP_FILE:
		{
			retval = gzgets( gzip.fp, s, size );
		}
		break;
	default:
		assert( false );
	}

	return retval;
}

int AbstractFile::seek( offset_t offset,
						int whence )
{
	int rc = 0;

	switch( type )
	{
	case TYPE_FILE:
		{
			rc = fseek( file.fp, offset, whence );
		}
		break;
	case TYPE_GZIP_FILE:
		{
			rc = gzseek( gzip.fp, offset, whence );
			if( rc >= 0 )
			{
				rc = 0;
			}
		}
		break;
	default:
		assert( false );
	}

	return rc;
}

AbstractFile::offset_t AbstractFile::tell()
{
	offset_t rc = 0;

	switch( type )
	{
	case TYPE_FILE:
		{
			rc = ftell( file.fp );
		}
		break;
	case TYPE_GZIP_FILE:
		{
			rc = gztell( gzip.fp );
		}
		break;
	default:
		assert( false );
	}

	return rc;
}

void AbstractFile::init( ConcreteFileType type,
						 const char *abstractPath,
						 const char *mode )
{
	this->type = type;
	this->abstractPath = strdup( abstractPath );

	switch( type )
	{
	case TYPE_FILE:
		{
			file.path = abstractPath;
			file.fp = fopen( file.path, mode );
			if( !file.fp )
			{
				fprintf( stderr, "Unable to open file at '%s'\n", abstractPath );
				perror( file.path );
				system( "echo hello from file path" );
				while( true )
					sleep(1);
			  #pragma omp critical(lsof)
			  {
				system( "lsof -u `whoami` > /tmp/lsof.txt" );
			  }
				assert( file.fp );
			}
		}
		break;
	case TYPE_GZIP_FILE:
		{
			gzip.path = createPath( TYPE_GZIP_FILE, abstractPath );
			gzip.fp = gzopen( gzip.path, mode );
			if( ! gzip.fp )
			{
				fprintf( stderr, "Unable to open file at '%s'\n", gzip.path );
				perror( gzip.path );
				system( "echo hello from gzip path" );
				while( true )
					sleep(1);
			  #pragma omp critical(lsof)
			  {
				system( "lsof -u `whoami` > /tmp/lsof.txt" );
			  }
				assert( gzip.fp );
			}
		}
		break;
	default:
		assert( false );
	}

}

char *AbstractFile::createPath( ConcreteFileType type, const char *abstractPath )
{
	char *path = NULL;

	switch( type )
	{
	case TYPE_FILE:
		{
			path = strdup( abstractPath );
			if( strstr( &path[strlen(path)-strlen(GZIP_EXT)], GZIP_EXT ) )
				path[strlen(path)-strlen(GZIP_EXT)] = '\0';
		}
		break;
	case TYPE_GZIP_FILE:
		{
			if( strstr( &abstractPath[strlen(abstractPath)-strlen(GZIP_EXT)], GZIP_EXT ) )
				path = strdup( abstractPath );
			else
			{
				path = (char *)malloc( strlen(abstractPath) + strlen(GZIP_EXT) + 1 );
				assert( path );
				sprintf( path, "%s%s", abstractPath, GZIP_EXT );
			}
		}
		break;
	default:
		assert( false );
	}

	return path;
}

void AbstractFile::test()
{
#define CLEANUP() \
	{int __rc = system( "rm -f file.out file.out.rename file.out.link gzip.out gzip.out.gz gzip.out.rename gzip.out.rename.zip gzip.out.link.gz" ); assert( __rc==0 );}

	CLEANUP();

	// --------------------
	// --- printf()
	// --------------------
	{
		AbstractFile gzip( AbstractFile::TYPE_GZIP_FILE, "gzip.out", "w" );
		AbstractFile file( AbstractFile::TYPE_FILE, "file.out", "w" );
		offset_t offset = -1;

		for( int i = 0; i < 100; i++ )
		{
			int rc_gzip;
			int rc_file;

			rc_gzip = gzip.printf( "i = %06d, i * 1.5 = %f\n", i, i * 1.5 );
			gzip.flush(false);

			rc_file = file.printf( "i = %06d, i * 1.5 = %f\n", i, i * 1.5 );
			file.flush(false);

			assert( rc_gzip == rc_file );
			assert( gzip.tell() == file.tell() );

			assert( file.tell() > offset );
			offset = file.tell();
		}

		gzip.close();
		file.close();

		int rc;
		
		rc = system( "gunzip gzip.out.gz" );
		assert( rc == 0 );

		rc = system( "diff file.out gzip.out" );
		assert( rc == 0 );

		rc = system( "gzip gzip.out" );
		assert( rc == 0 );
	}

	// --------------------
	// --- scanf()
	// --------------------
	{
		AbstractFile gzip( "gzip.out", "r" );
		AbstractFile file( "file.out", "r" );
		
		assert( gzip.type == TYPE_GZIP_FILE );
		assert( file.type == TYPE_FILE );

		for( int i = 0; i < 101; i++ )
		{
			int rc_gzip;
			int rc_file;
			int d_gzip;
			int d_file;
			float f_gzip;
			float f_file;

			rc_gzip = gzip.scanf( "i = %06d, i * 1.5 = %f\n", &d_gzip, &f_gzip );
			rc_file = file.scanf( "i = %06d, i * 1.5 = %f\n", &d_file, &f_file );

			assert( rc_gzip == rc_file );
			assert( gzip.tell() == file.tell() );

			if( i < 100 )
			{
				assert( d_gzip == d_file );
				assert( f_gzip == f_file );

				assert( f_gzip == (i * 1.5) );
				assert( d_gzip == i );
			}
			else
			{
				assert( rc_gzip == -1 );
			}
		}

		gzip.close();
		file.close();
	}

	// --------------------
	// --- read()
	// --------------------
	{
		AbstractFile gzip( "gzip.out", "r" );
		AbstractFile file( "file.out", "r" );
		
		assert( gzip.type == TYPE_GZIP_FILE );
		assert( file.type == TYPE_FILE );

		char buf_gzip[16];
		char buf_file[16];
		offset_t offset = 0;

		for( int i = 0; true; i++ )
		{
			size_t rc_gzip;
			size_t rc_file;

			rc_gzip = gzip.read( buf_gzip, 8, 2 );
			rc_file = file.read( buf_file, 8, 2 );

			assert( rc_gzip == rc_file );
			assert( gzip.tell() == file.tell() );

			// arbitrary number... just don't want to do this when underflow at end.
			if( i < 50 )
			{
				assert( rc_gzip == 2 );
				assert( 0 == memcmp(buf_gzip, buf_file, 16) );

				offset += 16;
				assert( gzip.tell() == offset );
			}

			if( rc_gzip == 0 )
			{
				break;
			}
		}

		gzip.close();
		file.close();
	}

	// --------------------
	// --- gets()
	// --------------------
	{
		AbstractFile gzip( "gzip.out", "r" );
		AbstractFile file( "file.out", "r" );
		
		assert( gzip.type == TYPE_GZIP_FILE );
		assert( file.type == TYPE_FILE );

		char buf_gzip[1024];
		char buf_file[1024];

		for( int i = 0; true; i++ )
		{
			char *s_gzip;
			char *s_file;

			s_gzip = gzip.gets( buf_gzip, sizeof(buf_gzip) );
			s_file = file.gets( buf_file, sizeof(buf_file) );

			assert( (s_gzip == NULL) == (s_file == NULL) );
			assert( gzip.tell() == file.tell() );

			if( s_gzip == NULL )
			{
				break;
			}

			assert( 0 == strcmp(s_gzip, s_file) );
			assert( NULL != strchr(s_gzip, '\n') );
		}

		gzip.close();
		file.close();
	}

	// --------------------
	// --- seek() & read()
	// --------------------
	{
		const size_t N = 32;

		AbstractFile gzip( "gzip.out", "r" );
		AbstractFile file( "file.out", "r" );
		
		assert( gzip.type == TYPE_GZIP_FILE );
		assert( file.type == TYPE_FILE );

		char buf_gzip[N];
		char buf_file[N];
		size_t rc_read_gzip;
		size_t rc_read_file;
		int rc_seek_gzip;
		int rc_seek_file;

		//
		// read N from beginning
		// 
		rc_read_gzip = gzip.read( buf_gzip, 1, N );
		rc_read_file = file.read( buf_file, 1, N );

		assert( rc_read_gzip == rc_read_file );
		assert( rc_read_gzip == N );
		assert( gzip.tell() == file.tell() );
		assert( gzip.tell() == (offset_t)N );
		assert( 0 == memcmp(buf_gzip, buf_file, N) );
		
		//
		// seek to 2*N after beginning
		// 
		rc_seek_gzip = gzip.seek( 2*N, SEEK_SET );
		rc_seek_file = file.seek( 2*N, SEEK_SET );

		assert( rc_seek_gzip == rc_seek_file );
		assert( rc_seek_gzip == 0 );
		assert( gzip.tell() == file.tell() );
		assert( gzip.tell() == 2*N );

		//
		// read N
		// 
		rc_read_gzip = gzip.read( buf_gzip, 1, N );
		rc_read_file = file.read( buf_file, 1, N );

		assert( rc_read_gzip == rc_read_file );
		assert( rc_read_gzip == N );
		assert( gzip.tell() == file.tell() );
		assert( gzip.tell() == 3*N );
		assert( 0 == memcmp(buf_gzip, buf_file, N) );

		//
		// skip N
		//
		rc_seek_gzip = gzip.seek( N, SEEK_CUR );
		rc_seek_file = file.seek( N, SEEK_CUR );

		assert( rc_seek_gzip == rc_seek_file );
		assert( rc_seek_gzip == 0 );
		assert( gzip.tell() == file.tell() );
		assert( gzip.tell() == 4*N );

		//
		// read N
		// 
		rc_read_gzip = gzip.read( buf_gzip, 1, N );
		rc_read_file = file.read( buf_file, 1, N );

		assert( rc_read_gzip == rc_read_file );
		assert( rc_read_gzip == N );
		assert( gzip.tell() == file.tell() );
		assert( gzip.tell() == 5*N );
		assert( 0 == memcmp(buf_gzip, buf_file, N) );

		gzip.close();
		file.close();
	}

	// --------------------
	// --- isCapable()
	// --------------------
	{
		AbstractFile gzip( "gzip.out", "r" );
		AbstractFile file( "file.out", "r" );
		
		assert( gzip.type == TYPE_GZIP_FILE );
		assert( file.type == TYPE_FILE );

		assert( !gzip.isCapable(CAP_SEEK_END) );
		assert( file.isCapable(CAP_SEEK_END) );

		assert( !gzip.isCapable(CAP_REWRITE) );
		assert( file.isCapable(CAP_REWRITE) );
	}

	// --------------------
	// --- exists()
	// --------------------
	{
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( exists("gzip.out") );
		assert( !exists(TYPE_FILE, "gzip.out") );

		assert( exists(TYPE_FILE, "file.out") );
		assert( exists("file.out") );
		assert( !exists(TYPE_GZIP_FILE, "file.out") );

		bool isAmbiguous;
		ConcreteFileType type;
		bool found;
		
		found = exists( "gzip.out", &isAmbiguous, &type );
		assert( found );
		assert( !isAmbiguous );
		assert( type == TYPE_GZIP_FILE );

		found = exists( "file.out", &isAmbiguous, &type );
		assert( found );
		assert( !isAmbiguous );
		assert( type == TYPE_FILE );

		int rc;
		rc = system( "gunzip -c gzip.out.gz > gzip.out" );
		assert( rc == 0 );

		found = exists( "gzip.out", &isAmbiguous, &type );
		assert( found );
		assert( isAmbiguous );
		assert( type == TYPE_UNDEFINED );

		rc = system( "rm gzip.out.gz" );
		assert( rc == 0 );

		found = exists( "gzip.out", &isAmbiguous, &type );
		assert( found );
		assert( !isAmbiguous );
		assert( type == TYPE_FILE );

		rc = system( "gzip gzip.out" );
		assert( rc == 0 );

		found = exists( "gzip.out", &isAmbiguous, &type );
		assert( found );
		assert( !isAmbiguous );
		assert( type == TYPE_GZIP_FILE );		
	}

	// --------------------
	// --- link()
	// --------------------
	{
		int rc;

		rc = link( "file.out", "file.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "file.out.link") );

		rc = link( "file.out", "file.out.link" );
		assert( rc == -1 );
		assert( exists(TYPE_FILE, "file.out.link") );
		
		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out.link.gz") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == -1 );
		assert( exists(TYPE_FILE, "gzip.out.link.gz") );
		
		rc = system( "rm gzip.out.link.gz" );
		assert( rc == 0 );

		rc = system( "gunzip -c gzip.out.gz > gzip.out" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == -1 );

		rc = system( "rm gzip.out" );
		assert( rc == 0 );
		assert( !exists(TYPE_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out.link.gz") );

		rc = system( "rm -f file.out.link gzip.out.link.gz" );
		assert( rc == 0 );
	}

	// --------------------
	// --- unlink()
	// --------------------
	{
		int rc;

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out.link") );

		rc = unlink( "gzip.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out.link") );

		rc = link( "file.out", "file.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "file.out") );
		assert( exists(TYPE_FILE, "file.out.link") );

		rc = unlink( "file.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "file.out") );
		assert( !exists(TYPE_FILE, "file.out.link") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == 0 );

		rc = system( "gunzip -c gzip.out.link.gz > gzip.out.link" );
		assert( rc == 0 );

		rc = unlink( "gzip.out.link" );
		assert( rc == -1 );

		rc = system( "rm gzip.out.link" );
		assert( rc == 0 );

		rc = unlink( "gzip.out.link" );
		assert( rc == 0 );

		rc = system( "rm -f file.out.link gzip.out.link.gz" );
		assert( rc == 0 );
	}

	// --------------------
	// --- rename()
	// --------------------
	{
		int rc;

		rc = rename( "file.out", "file.out.rename" );
		assert( rc == 0 );
		assert( !exists(TYPE_FILE, "file.out") );
		assert( exists(TYPE_FILE, "file.out.rename") );

		rc = rename( "file.out.rename", "file.out" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "file.out") );
		assert( !exists(TYPE_FILE, "file.out.rename") );

		rc = rename( "file.out.rename", "file.out" );
		assert( rc == -1 );
		assert( exists(TYPE_FILE, "file.out") );
		assert( !exists(TYPE_FILE, "file.out.rename") );

		rc = rename( "gzip.out", "gzip.out.rename" );
		assert( rc == 0 );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out.rename") );

		rc = rename( "gzip.out.rename", "gzip.out" );
		assert( rc == 0 );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out.rename") );

		rc = rename( "gzip.out.rename", "gzip.out" );
		assert( rc == -1 );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out.rename") );

		rc = system( "gunzip -c gzip.out.gz > gzip.out" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );

		rc = rename( "gzip.out", "gzip.out.rename" );
		assert( rc == -1 );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out.rename") );

		rc = system( "rm gzip.out" );
		assert( rc == 0 );

		rc = rename( "gzip.out", "gzip.out.rename" );
		assert( rc == 0 );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out.rename") );

		rc = rename( "gzip.out.rename", "gzip.out" );
		assert( rc == 0 );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );
		assert( !exists(TYPE_GZIP_FILE, "gzip.out.rename") );

		/*
		rc = link( "file.out", "file.out.link" );
		assert( rc == -1 );
		assert( exists(TYPE_FILE, "file.out.link") );
		
		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out.link.gz") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == -1 );
		assert( exists(TYPE_FILE, "gzip.out.link.gz") );
		
		rc = system( "rm gzip.out.link.gz" );
		assert( rc == 0 );

		rc = system( "gunzip -c gzip.out.gz > gzip.out" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == -1 );

		rc = system( "rm gzip.out" );
		assert( rc == 0 );
		assert( !exists(TYPE_FILE, "gzip.out") );
		assert( exists(TYPE_GZIP_FILE, "gzip.out") );

		rc = link( "gzip.out", "gzip.out.link" );
		assert( rc == 0 );
		assert( exists(TYPE_FILE, "gzip.out.link.gz") );

		rc = system( "rm -f file.out.link gzip.out.link.gz" );
		assert( rc == 0 );
		*/
	}

	CLEANUP();
}
