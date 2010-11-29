#pragma once

#include <stdio.h>
#include <zlib.h>

class AbstractFile
{
 public:
	enum ConcreteFileType
	{
		TYPE_UNDEFINED,
		TYPE_FILE,
		TYPE_GZIP_FILE
	};
	enum ConcreteFileCapability
	{
		CAP_SEEK_END,
		CAP_REWRITE
	};

	typedef long offset_t;

	static AbstractFile *open( ConcreteFileType type,
							   const char *abstractPath,
							   const char *mode );
	static AbstractFile *open( const char *abstractPath,
							   const char *mode );

	static bool exists( const char *abstractPath,
						bool *isAmbiguous = NULL,
						ConcreteFileType *typeFound = NULL );
	static bool exists( ConcreteFileType type,
						const char *abstractPath );

	static int link( const char *oldAbstractPath,
					 const char *newAbstractPath );

	static int unlink( const char *abstractPath );

	static int rename( const char *oldAbstractPath,
					   const char *newAbstractPath );

	AbstractFile( ConcreteFileType type,
				  const char *abstractPath,
				  const char *mode );

	AbstractFile( const char *abstractPath,
				  const char *mode );

	virtual ~AbstractFile();

	int close();

	const char *getAbstractPath();
	
	bool isCapable( ConcreteFileCapability cap );

	size_t write( const void *ptr, size_t size, size_t nmemb );
	int printf( const char *format, ... );
	int flush( bool full = false );

	size_t read( void *ptr, size_t size, size_t nmemb );
	int scanf( const char *format, ... );
	char *gets( char *s, int size );

	int seek( offset_t offset, int whence );
	offset_t tell();

 private:
	void init( ConcreteFileType type,
			   const char *abstractPath,
			   const char *mode );
	static char *createPath( ConcreteFileType type, const char *abstractPath );

	ConcreteFileType type;
	const char *abstractPath;
	union
	{
		struct
		{
			const char *path;
			FILE *fp;
		} file;
		
		struct
		{
			const char *path;
			gzFile fp;
		} gzip;
	};

 public:
	static void test();
};
