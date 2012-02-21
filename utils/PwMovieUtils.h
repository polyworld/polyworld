#ifndef PwMovieUtils_h
#define PwMovieUtils_h

#include <stdint.h>
#include <stdio.h>

#include <list>
#include <map>
#include <string>

/* #define PLAINRLE */

// When bumping the movie version, just bump kCurrentMovieVersionHost.
// A platform/processor/endian-specific version will be determined
//from this host version automatically.
#ifdef PLAINRLE
	#define kCurrentMovieVersionHost 1
#else
	#define kCurrentMovieVersionHost 6
#endif

#if __BIG_ENDIAN__
	#define kCurrentMovieVersion kCurrentMovieVersionHost
#else
	#define kCurrentMovieVersion (kCurrentMovieVersionHost+100)
#endif

void PwRecordMovie( FILE *f, long xleft, long ybottom, long width, long height );

void PwReadMovieHeader ( FILE *f, uint32_t* version, uint32_t* width, uint32_t* height );
void PwWriteMovieHeader( FILE *f, uint32_t  version, uint32_t  width, uint32_t  height );


//===========================================================================
// PwMovieFileHeader
//===========================================================================
#pragma pack(push, 1)
struct PwMovieFileHeader
{
	uint32_t version;
	uint32_t sizeofHeader;
	uint32_t frameCount;
	uint32_t metaEntryCount;
	uint64_t offsetMetaEntries;
};

//===========================================================================
// PwMovieMetaEntry
//===========================================================================
namespace PwMovieMetaEntry
{
	enum Type
	{
		DIMENSIONS = 0,
		TIMESTEP,
		CHECKPOINT,
		__NTYPES
	};

	struct FileHeader
	{
		uint8_t type;
		uint32_t frame;
		uint32_t sizeBody;
	};

	struct Dimensions
	{
		uint32_t width;
		uint32_t height;
	};

	struct Timestep
	{
		uint32_t timestep;
	};

	struct Checkpoint
	{
		uint64_t offsetFrame;
	};

	struct Entry
	{
		FileHeader header;

		union
		{
			uint8_t *__body;
			Dimensions *dimensions;
			Timestep *timestep;
			Checkpoint *checkpoint;
		};

		void dispose();
	};
}
#pragma pack(pop)

//===========================================================================
// PwMovieWriter
//===========================================================================
class PwMovieWriter
{
 public:
	PwMovieWriter( FILE *file );
	~PwMovieWriter();

	void writeFrame( uint32_t timestep,
					 uint32_t width,
					 uint32_t height,
					 uint32_t *rgbBufOld,
					 uint32_t *rgbBufNew );

	void close();

 private:
	void writeHeader();
	void setDimensions( uint32_t width,
						uint32_t height );
	void setTimestep( uint32_t timestep );
	void addCheckpoint();

	void writeRleFrame( uint32_t *rgbBuf );
	void writeRleDiffFrame( uint32_t *rgbBufOld, uint32_t *rgbBufNew );

	FILE *file;
	PwMovieFileHeader header;
	uint32_t frame;
	uint32_t timestep;
	uint32_t width;
	uint32_t height;
	uint32_t *rleBuf;
	uint32_t rleBufSize;
	typedef std::list<PwMovieMetaEntry::Entry> EntryList;
	EntryList metaEntries;
};

//===========================================================================
// PwMovieReader
//===========================================================================
class PwMovieReader
{
 public:
	PwMovieReader( FILE *file );
	~PwMovieReader();

	uint32_t getFrameCount();
	void readFrame( uint32_t frame,
					uint32_t *ret_timestep,
					uint32_t *ret_width,
					uint32_t *ret_height,
					uint32_t **ret_rgbBuf );

 private:
	void readHeader();
	PwMovieMetaEntry::Entry *findMeta( uint32_t frame,
									   PwMovieMetaEntry::Type type,
									   bool searchPreviousFrames = false );
	void setDimensions( uint32_t width, uint32_t height );
	void seekFrame( uint32_t frame );
	void nextFrame();

	FILE *file;
	PwMovieFileHeader header;
	uint32_t frame;
	uint32_t *rgbBuf;
	uint32_t *rleBuf;
	uint32_t width;
	uint32_t height;

	// descending order sort so we can use lower_bound to find entry <= current frame.
	typedef std::map<uint32_t, PwMovieMetaEntry::Entry *, std::greater<uint32_t> > FrameMetaEntryMap;

	FrameMetaEntryMap metaEntries[ PwMovieMetaEntry::__NTYPES ];
};

//===========================================================================
// PwMovieQGLWidgetRecorder
//===========================================================================
class PwMovieQGLWidgetRecorder
{
 public:
	PwMovieQGLWidgetRecorder( class QGLWidget *widget, PwMovieWriter *writer );
	~PwMovieQGLWidgetRecorder();
	
	void recordFrame( uint32_t timestep );

 private:
	void setDimensions();

	class QGLWidget *widget;
	PwMovieWriter *writer;
	uint32_t width;
	uint32_t height;
	uint32_t *rgbBufOld;
	uint32_t *rgbBufNew;
};

//===========================================================================
// PwMovieQGLPixelBufferRecorder
//===========================================================================
class PwMovieQGLPixelBufferRecorder
{
 public:
	PwMovieQGLPixelBufferRecorder( class QGLPixelBuffer *pixelBuffer, PwMovieWriter *writer );
	~PwMovieQGLPixelBufferRecorder();
	
	void recordFrame( uint32_t timestep );

 private:
	class QGLPixelBuffer *pixelBuffer;
	PwMovieWriter *writer;
	uint32_t width;
	uint32_t height;
	uint32_t *rgbBufOld;
	uint32_t *rgbBufNew;
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
