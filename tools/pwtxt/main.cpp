#include "stdio.h"

#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>

#include "PositionPath.h"
#include "PositionRecord.h"

using namespace std;

class Options {
public:
	Options() {
		recursive = false;
		verbose = false;
	}

public:
	bool recursive;
	bool verbose;
} opts;

void err( string msg, int exit_value = 1 );
void warn( string msg );

bool isdir( const char *path );
bool isfile( const char *path );
bool isbin( const char *path );

void process_path( const char *path, int depth );
void process_dir( const char *path, int depth );
void process_file( const char *path );

string maketxt( const char *path );

void convert_position( const char *path );

void show_usage();

int main( int argc, char **argv )
{
	if( argc == 1 )
	{
		show_usage();
	}

	const char *opts_spec_short = "rRv";
	string opts_spec = opts_spec_short;
	
	char c;
	while( (c = getopt( argc, argv, opts_spec.c_str() )) != -1 )
	{
		switch( c )
		{
		case 'r':
		case 'R':
			opts.recursive = true;
			break;
		case 'v':
			opts.verbose = true;
			break;
		default:
			char s[] = {optopt, 0};
			err( string( "Invalid option: " ) + s );
		}
	}

	if( optind == argc )
	{
		err( "Must provide at least one file/dir" );
	}

	for( int i = optind; i < argc; i++ )
	{
		const char *path = argv[i];

		process_path( path, 0 );
	}

	return 0;
}

void err( string msg, int exit_value )
{
	fprintf( stderr, "ERROR! %s\n", msg.c_str() );
	exit( exit_value );
}

void warn( string msg )
{
	fprintf( stderr, "WARNING! %s\n", msg.c_str() );
}

bool isdir( const char *path )
{
	struct stat _stat;

	int rc = stat( path, &_stat );
	if( rc != 0 ) return false;

	return S_ISDIR( _stat.st_mode );
}

bool isfile( const char *path )
{
	struct stat _stat;

	int rc = stat( path, &_stat );
	if( rc != 0 ) return false;

	return S_ISREG( _stat.st_mode );
}

bool isbin( const char *path )
{
	int n = strlen( path );

	if( n < 4 )
	{
		return false;
	}

	return 0 == strcasecmp( path + n - 4, ".bin" );
}

void process_path( const char *path, int depth )
{
	if( isdir( path ) )
	{
		if( opts.recursive || (depth <= 1) )
		{
			process_dir( path, depth );
		}
	}
	else if( isfile( path ) )
	{
		process_file( path );
	}
	else
	{
		err( string( "Not a valid file: " ) + path );
	}
}

void process_dir( const char *path, int depth )
{
	DIR *dir = opendir( path );
	if( !dir )
	{
		err( string( "opendir() failed on " ) + path );
	}

	dirent *ent;
	while( ent = readdir( dir ) )
	{
		if( 0 != strcmp( ".", ent->d_name )
			&& 0 != strcmp( "..", ent->d_name ) )
		{
			string path_ent = string( path ) + "/" + ent->d_name;

			process_path( path_ent.c_str(), depth + 1 );
		}
	}

	closedir( dir );
}

void process_file( const char *path )
{
	if( isbin( path ) )
	{
		char *name = rindex( path, '/' );

		if( strstr( name, PATH__BASENAME_POSITION ) )
		{
			convert_position( path );
		}
	}
}

string maketxt( const char *path )
{
	return string( path ).substr( 0, strlen( path ) - 4 ) + ".txt";
}

void convert_position( const char *path )
{
	string pathtxt = maketxt( path );

	if( opts.verbose )
	{
		printf("%s --> %s\n", path, pathtxt.c_str() );
	}

	FILE *fin = fopen( path, "r" );
	assert( fin );

	FILE *fout = fopen( pathtxt.c_str(), "w" );
	assert( fout );

	PositionHeader hdr;
	size_t rc = fread( &hdr,
					   sizeof(PositionHeader),
					   1,
					   fin );
	assert( rc == 1 );

	assert( hdr.sizeof_header == sizeof( hdr ) );
	assert( hdr.sizeof_record == sizeof( PositionRecord ) );
		  
	fprintf( fout, "# %10s%12s%12s%12s\n", "TIMESTEP", "X", "Y", "Z" );

	PositionRecord record;
	while( 1 == fread( &record,
					   sizeof( record ),
					   1,
					   fin ) )
	{
		fprintf( fout,
				 "%12ld%12f%12f%12f\n",
				 record.step, record.x, record.y, record.z );
	}

	fclose( fin );
	fclose( fout );
}

void show_usage()
{
#define nl() printf("\n")
#define p(x...) printf(x); nl()

	p("USAGE");
	nl();
	p("     pwtxt [<option>...] <path>...");
	nl();
	p("DESCRIPTION");
	nl();
	p("     Converts binary data files to text. <path> can be a directory or a regular");
	p("  file.");
	nl();
	p("OPTIONS");
	nl();
	p("     -r,-R     Recurse through directories.");
	nl();
	p("     -v        Verbose processing information.");

#undef p

	exit( 1 );
}
// eof
