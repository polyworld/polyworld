#ifndef PwMovieTools_h
#define PwMovieTools_h

#include <stdio.h>

/* #define PLAINRLE */

// When bumping the movie version, just bump kCurrentMovieVersionHost.
// A platform/processor/endian-specific version will be determined
//from this host version automatically.
#ifdef PLAINRLE
	#define kCurrentMovieVersionHost 1
#else
	#define kCurrentMovieVersionHost 5
#endif

#if __BIG_ENDIAN__
	#define kCurrentMovieVersion kCurrentMovieVersionHost
#else
	#define kCurrentMovieVersion (kCurrentMovieVersionHost+100)
#endif

void PwRecordMovie( FILE *f, long xleft, long ybottom, long width, long height );

void PwReadMovieHeader ( FILE *f, unsigned long* version, unsigned long* width, unsigned long* height );
void PwWriteMovieHeader( FILE *f, unsigned long  version, unsigned long  width, unsigned long  height );

void rleproc( register unsigned long *rgb,
			  register unsigned long width,
			  register unsigned long height,
			  register unsigned long *rle,
			  register unsigned long rleBufSize );

void rlediff2( register unsigned long *rgbnew,
			   register unsigned long *rgbold,
			   register unsigned long width,
			   register unsigned long height,
			   register unsigned long *rle,
			   register unsigned long rleBufSize );

void rlediff3( register unsigned long *rgbnew,
               register unsigned long *rgbold,
			   register unsigned long width,
			   register unsigned long height,
               register unsigned long *rle,
			   register unsigned long rleBufSize );

void rlediff4( register unsigned long *rgbnew,
               register unsigned long *rgbold,
			   register unsigned long width,
			   register unsigned long height,
               register unsigned long *rle,
			   register unsigned long rleBufSize );

void unrle( register unsigned long *rle,
			register unsigned long *rgb,
			register unsigned long width,
			register unsigned long height,
			register unsigned long version );

void unrlediff2( register unsigned long *rle,
				 register unsigned long *rgb,
				 register unsigned long width,
				 register unsigned long height,
				 register unsigned long version );

void unrlediff3( register unsigned long *rle,
				 register unsigned long *rgb,
				 register unsigned long width,
				 register unsigned long height,
				 register unsigned long version );

void unrlediff4( register unsigned long *rle,
				 register unsigned long *rgb,
				 register unsigned long width,
				 register unsigned long height,
				 register unsigned long version );

int readrle( register FILE *f, register unsigned long *rle, register unsigned long version, register bool firstFrame );

char* sgets( char* string, size_t size, FILE* file );
double hirestime( void );

#endif
