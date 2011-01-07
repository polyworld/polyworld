#ifndef PwMovieUtils_h
#define PwMovieUtils_h

#include <stdint.h>
#include <stdio.h>

#include <list>
#include <string>

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

class PwMovieIndexer
{
 public:
	// ------------------------------------------------------------
	// --- WRITING
	// ------------------------------------------------------------
	static void createIndexFile( const std::string &path_movie, uint32_t indexStride );

 public:
	// ------------------------------------------------------------
	// --- READING
	// ------------------------------------------------------------
	PwMovieIndexer( const std::string &path_movie );
	~PwMovieIndexer();

	uint32_t getFrameCount();

	void findFrame( uint32_t frame,
					FILE *fileMovie,
					uint32_t *rleBuf,
					uint32_t *rgbBuf );

 private:
	static std::string getIndexPath( const std::string &path_movie );

#pragma pack(push, 1)
	struct FileHeader
	{
		uint32_t signature;
		uint32_t sizeofHeader;
		uint32_t sizeofEntry;
		uint32_t version;
		uint32_t width;
		uint32_t height;
		uint32_t movieVersion;
		uint32_t frameCount;
		uint32_t indexStride;
		uint32_t entryCount;
		uint64_t offsetEntries;
	};
	struct Entry
	{
		uint32_t frameNumber;
		uint64_t offsetIndexFrame;
		uint64_t offsetMovieNextFrame;
		uint32_t sizeIndexFrame;
	};
#pragma pack(pop)

	FILE *fileIndex;
	FileHeader header;
};

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
