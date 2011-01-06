#ifndef PwMovieUtils_h
#define PwMovieUtils_h

#include <stdint.h>
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

void PwReadMovieHeader ( FILE *f, uint32_t* version, uint32_t* width, uint32_t* height );
void PwWriteMovieHeader( FILE *f, uint32_t  version, uint32_t  width, uint32_t  height );

void rleproc( register uint32_t *rgb,
			  register uint32_t width,
			  register uint32_t height,
			  register uint32_t *rle,
			  register uint32_t rleBufSize );

void rlediff2( register uint32_t *rgbnew,
			   register uint32_t *rgbold,
			   register uint32_t width,
			   register uint32_t height,
			   register uint32_t *rle,
			   register uint32_t rleBufSize );

void rlediff3( register uint32_t *rgbnew,
               register uint32_t *rgbold,
			   register uint32_t width,
			   register uint32_t height,
               register uint32_t *rle,
			   register uint32_t rleBufSize );

void rlediff4( register uint32_t *rgbnew,
               register uint32_t *rgbold,
			   register uint32_t width,
			   register uint32_t height,
               register uint32_t *rle,
			   register uint32_t rleBufSize );

void unrle( register uint32_t *rle,
			register uint32_t *rgb,
			register uint32_t width,
			register uint32_t height,
			register uint32_t version );

void unrlediff2( register uint32_t *rle,
				 register uint32_t *rgb,
				 register uint32_t width,
				 register uint32_t height,
				 register uint32_t version );

void unrlediff3( register uint32_t *rle,
				 register uint32_t *rgb,
				 register uint32_t width,
				 register uint32_t height,
				 register uint32_t version );

void unrlediff4( register uint32_t *rle,
				 register uint32_t *rgb,
				 register uint32_t width,
				 register uint32_t height,
				 register uint32_t version );

int readrle( register FILE *f, register uint32_t *rle, register uint32_t version, register bool firstFrame );

char* sgets( char* string, size_t size, FILE* file );
double hirestime( void );

#endif
