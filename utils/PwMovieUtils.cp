// WARNING:  rlediff2 and rlediff3 have NOT been ported to the Mac's RGBA color format.
// (They retain their original ABGR color format structure from the Iris.)
// Only rlediff4 and rleproc, the algorithms currently in use, have been updated to RGBA.
//
// However, all the unrle* routines have been modified to handle either old ABGR data,
// when the version number is 4 or less, or new RGBA data, when the version number is
// 5 or greater.

#define _FILE_OFFSET_BITS 64 // tell gnu to use 64 bit file offsets for ftello

#define PMP_DEBUG 0

#define CHECKPOINT_STRIDE 500

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if __APPLE__
	#include <mach/mach_time.h>
	#include <libkern/OSByteOrder.h>
#else	// #elif linux
	#include <byteswap.h>
	#include <netinet/in.h>
	#include <sys/time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include <gl.h>
#include <QGLWidget>

#include "misc.h"
#include "PwMovieUtils.h"

using namespace std;

#define HIGHBITON 0x80000000
#define HIGHBITOFF 0x7fffffff
#define HIGHBITONSHORT 0x8000
#define HIGHBITOFFSHORT 0x7fff

#if __BIG_ENDIAN__
	#define AlphaMask_ABGR 0xff000000
	#define AlphaMask_RGBA 0x000000ff
	#define NoAlphaMask_ABGR 0x00ffffff
	#define NoAlphaMask_RGBA 0xffffff00
#else
	#define AlphaMask_ABGR 0x000000ff
	#define AlphaMask_RGBA 0xff000000
	#define NoAlphaMask_ABGR 0xffffff00
	#define NoAlphaMask_RGBA 0x00ffffff
#endif

#if PMP_DEBUG
	#define pmpPrint( x... ) { printf( "%s: ", __FUNCTION__ ); printf( x ); }
    #define pmpdb( stmt ) stmt;
#else
	#define pmpPrint( x... )
    #define pmpdb( stmt )
#endif

#if __APPLE__
	#define SwapInt32(x) OSSwapInt32(x)
	#define SwapInt16(x) OSSwapInt16(x)
#else	// #elif linux
	#define SwapInt32(x) bswap_32(x)
	#define SwapInt16(x) bswap_16(x)
#endif

namespace PwMovieMetaEntry
{
	void Entry::dispose()
	{
		delete __body;
	}
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---
//--- PwMovieWriter
//---
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
PwMovieWriter::PwMovieWriter( FILE *file )
{
#if __BIG_ENDIAN__
	fprintf( stderr, "big endian arch not currently supported for movie files.\n" );
	exit( 1 );
#endif

	this->file = file;
	
	frame = 0;
	timestep = 1;
	width = 0;
	height = 0;
	rleBuf = NULL;
	rleBufSize = 0;

	// version always stored in big endian
	header.version = htonl( kCurrentMovieVersion );

	header.sizeofHeader = sizeof(header);
	header.metaEntryCount = 0;
	header.offsetMetaEntries = 0;

	writeHeader();
}

PwMovieWriter::~PwMovieWriter()
{
	close();

	if( rleBuf )
	{
		free( rleBuf );
		rleBuf = NULL;
	}
}

void PwMovieWriter::writeFrame( uint32_t timestep,
								uint32_t width,
								uint32_t height,
								uint32_t *rgbBufOld,
								uint32_t *rgbBufNew )
{
	assert( timestep > 0 );
	assert( (width > 0) && (height > 0) );

	frame++;

	bool useDiff = true;

	if( (width != this->width) || (height != this->height) )
	{
		useDiff = false;
		setDimensions( width, height );
	}

	if( (timestep != (this->timestep + 1)) || (frame == 1) )
	{
		useDiff = false;
		setTimestep( timestep );
	}

	if( ((frame - 1) % CHECKPOINT_STRIDE) == 0 )
	{
		useDiff = false;
		addCheckpoint();
	}

	this->timestep = timestep;

	if( useDiff )
	{
		writeRleDiffFrame( rgbBufOld, rgbBufNew );
	}
	else
	{
		writeRleFrame( rgbBufNew );
	}
}

void PwMovieWriter::close()
{
	if( file )
	{
		header.frameCount = frame;
		header.metaEntryCount = metaEntries.size();
		header.offsetMetaEntries = (uint64_t)ftello( file );
	
		itfor( EntryList, metaEntries, it )
		{
			PwMovieMetaEntry::Entry entry = *it;
			fwrite( &(entry.header), sizeof(entry.header), 1, file );
			fwrite( entry.__body, entry.header.sizeBody, 1, file );
			entry.dispose();
		}

		writeHeader();

		fclose( file );

		file = NULL;
		metaEntries.clear();
	}
}

void PwMovieWriter::writeHeader()
{
	fseeko( file, 0, SEEK_SET );
	fwrite( &header, sizeof(header), 1, file );
}

void PwMovieWriter::setDimensions( uint32_t width,
								   uint32_t height )
{
	this->width = width;
	this->height = height;

	if( rleBuf ) free( rleBuf );
	rleBufSize = 1 + width * height * sizeof(*rleBuf);
	rleBuf = (uint32_t*) malloc( rleBufSize );

	PwMovieMetaEntry::Entry entry;
	entry.header.type = PwMovieMetaEntry::DIMENSIONS;
	entry.header.frame = frame;
	entry.header.sizeBody = sizeof(*entry.dimensions);
	entry.dimensions = new PwMovieMetaEntry::Dimensions();
	entry.dimensions->width = width;
	entry.dimensions->height = height;

	metaEntries.push_back( entry );
}

void PwMovieWriter::setTimestep( uint32_t timestep )
{
	PwMovieMetaEntry::Entry entry;
	entry.header.type = PwMovieMetaEntry::TIMESTEP;
	entry.header.frame = frame;
	entry.header.sizeBody = sizeof(*entry.timestep);
	entry.timestep = new PwMovieMetaEntry::Timestep();
	entry.timestep->timestep = timestep;

	metaEntries.push_back( entry );
}

void PwMovieWriter::addCheckpoint()
{
	PwMovieMetaEntry::Entry entry;
	entry.header.type = PwMovieMetaEntry::CHECKPOINT;
	entry.header.frame = frame;
	entry.header.sizeBody = sizeof(*entry.checkpoint);
	entry.checkpoint = new PwMovieMetaEntry::Checkpoint();
	entry.checkpoint->offsetFrame = (uint64_t)ftello( file );

	metaEntries.push_back( entry );
}

void PwMovieWriter::writeRleFrame( uint32_t *rgbBuf )
{
	rleproc( rgbBuf, width, height, rleBuf, rleBufSize );

	uint32_t rleDataSize = sizeof(uint32_t) * (rleBuf[0] + 1);

	pmpdb( cout << " writing frame to offset " << ftello( file ) << ", datasize=" << rleDataSize << ", rleBuf[0]=" << rleBuf[0] << endl );

	fwrite( rleBuf, rleDataSize, 1, file );
}

void PwMovieWriter::writeRleDiffFrame( uint32_t *rgbBufOld,
									   uint32_t *rgbBufNew )
{
	rlediff4( rgbBufNew, rgbBufOld, width, height, rleBuf, rleBufSize );

	uint32_t rleDataSize = sizeof(uint16_t) * (rleBuf[0] + 2);

	pmpdb( cout << " writing frame to offset " << ftello( file ) << endl );

	fwrite( rleBuf, rleDataSize, 1, file );
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---
//--- PwMovieReader
//---
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
PwMovieReader::PwMovieReader( FILE *file )
{
#if __BIG_ENDIAN__
	fprintf( stderr, "big endian arch not currently supported for movie files.\n" );
	exit( 1 );
#endif

	this->file = file;
	frame = 0;
	rgbBuf = NULL;
	rleBuf = NULL;
	width = 0;
	height = 0;

	readHeader();
}

PwMovieReader::~PwMovieReader()
{
	for( int type = 0; type < (int)PwMovieMetaEntry::__NTYPES; type++ )
	{
		itfor( FrameMetaEntryMap, metaEntries[type], it )
		{
			it->second->dispose();
			delete it->second;
		}
	}
}

uint32_t PwMovieReader::getFrameCount()
{
	return header.frameCount;
}

void PwMovieReader::readFrame( uint32_t frame,
							   uint32_t *ret_timestep,
							   uint32_t *ret_width,
							   uint32_t *ret_height,
							   uint32_t **ret_rgbBuf )
{
	assert( (frame > 0) && (frame <= getFrameCount()) );

	pmpdb( cout << "readFrame(" << frame << ")" << endl );

	if( (this->frame == 0) || (this->frame != frame ) )
	{
		seekFrame( frame );
	}
	else
	{
		nextFrame();
	}

	// update timestep
	{
		PwMovieMetaEntry::Entry *entry = findMeta( frame,
												   PwMovieMetaEntry::TIMESTEP,
												   true );
		*ret_timestep = (frame - entry->header.frame) + (entry->timestep->timestep);
	}

	*ret_width = width;
	*ret_height = height;
	*ret_rgbBuf = rgbBuf;	
}

void PwMovieReader::readHeader()
{
	fseek( file, 0, SEEK_SET );
	fread( &header.version, sizeof(header.version), 1, file );
	uint32_t version = ntohl( header.version );

	if( version < 100 )
	{
		fprintf( stderr, "big endian movie files not currently supported.\n" );
		exit( 1 );
	}

	if( version >= 106 )
	{
		fseek( file, 0, SEEK_SET );
		fread( &header, sizeof(header), 1, file );

		assert( header.sizeofHeader == sizeof(header) );
		assert( header.metaEntryCount > 0 );
		assert( header.frameCount > 0 );

		pmpdb( cout << "frame count = " << header.frameCount << endl );

		fseeko( file, header.offsetMetaEntries, SEEK_SET );

		for( uint32_t i = 0; i < header.metaEntryCount; i++ )
		{
			PwMovieMetaEntry::Entry *entry = new PwMovieMetaEntry::Entry();
			fread( &entry->header, sizeof(entry->header), 1, file );

			pmpdb( cout << "entry " << i << " type=" << (int)entry->header.type << ", frame=" << entry->header.frame << ", sizeBody=" << entry->header.sizeBody << endl );
		
			entry->__body = new uint8_t[ entry->header.sizeBody ];
			fread( entry->__body, entry->header.sizeBody, 1, file );

			switch( entry->header.type )
			{
			case PwMovieMetaEntry::DIMENSIONS:
				pmpdb( cout << "  width=" << entry->dimensions->width << ", height=" << entry->dimensions->height << endl );
				break;
			}

			assert( entry->header.type < PwMovieMetaEntry::__NTYPES );
			metaEntries[ entry->header.type ][ entry->header.frame ] = entry;
		}
	}
	else
	{
		header.sizeofHeader = 0;
		header.frameCount = ~0;
		header.metaEntryCount = 0;
		header.offsetMetaEntries = 0;

		PwMovieMetaEntry::Entry *entry;

		// Create Dimensions meta entry for frame 1
		entry = new PwMovieMetaEntry::Entry();
		entry->header.type = PwMovieMetaEntry::DIMENSIONS;
		entry->header.frame = 1;
		entry->header.sizeBody = sizeof(PwMovieMetaEntry::Dimensions);
		entry->dimensions = new PwMovieMetaEntry::Dimensions();
		fread( entry->dimensions, sizeof(entry->dimensions), 1, file );
		metaEntries[ entry->header.type ][ 1 ] = entry;

		// Create Checkpoint meta entry for frame 1
		entry = new PwMovieMetaEntry::Entry();
		entry->header.type = PwMovieMetaEntry::CHECKPOINT;
		entry->header.frame = 1;
		entry->header.sizeBody = sizeof(PwMovieMetaEntry::Checkpoint);
		entry->checkpoint = new PwMovieMetaEntry::Checkpoint();
		entry->checkpoint->offsetFrame = ftello( file );
		metaEntries[ entry->header.type ][ 1 ] = entry;

		// Create Timestep meta entry for frame 1
		entry = new PwMovieMetaEntry::Entry();
		entry->header.type = PwMovieMetaEntry::TIMESTEP;
		entry->header.frame = 1;
		entry->header.sizeBody = sizeof(PwMovieMetaEntry::Timestep);
		entry->timestep = new PwMovieMetaEntry::Timestep();
		entry->timestep->timestep = 1;
		metaEntries[ entry->header.type ][ 1 ] = entry;
	}
}

PwMovieMetaEntry::Entry *PwMovieReader::findMeta( uint32_t frame,
												  PwMovieMetaEntry::Type type,
												  bool searchPreviousFrames )
{
	FrameMetaEntryMap::iterator it;
	FrameMetaEntryMap &map = metaEntries[ type ];
	if( searchPreviousFrames )
	{
		it = map.lower_bound( frame );
	}
	else
	{
		it = map.find( frame );
	}

	if( it == map.end() )
	{
		return NULL;
	}
	else
	{
		return it->second;
	}
}

void PwMovieReader::setDimensions( uint32_t width, uint32_t height )
{
	this->width = width;
	this->height = height;

	if( rleBuf ) free( rleBuf );
	if( rgbBuf ) free( rgbBuf );

	uint32_t rgbBufSize = width * height * sizeof(uint32_t);
	uint32_t rleBufSize = rgbBufSize + 1;

	rleBuf = (uint32_t *)malloc( rleBufSize );
	rgbBuf = (uint32_t *)malloc( rgbBufSize );
}

void PwMovieReader::seekFrame( uint32_t frame )
{
	PwMovieMetaEntry::Entry *entryCheckpoint = findMeta( frame,
														 PwMovieMetaEntry::CHECKPOINT,
														 true );

	fseeko( file, (off_t)entryCheckpoint->checkpoint->offsetFrame, SEEK_SET );
	this->frame = entryCheckpoint->header.frame;

	while( this->frame <= frame )
	{
		nextFrame();
	}
}

void PwMovieReader::nextFrame()
{
	bool diff = true;
	bool changedDimensions = false;

	pmpdb( cout << "start of nextFrame(), width=" << width << ", height=" << height << endl );

	// update dimensions
	{
		PwMovieMetaEntry::Entry *entry = findMeta( frame,
												   PwMovieMetaEntry::DIMENSIONS,
												   true );
		uint32_t entryWidth = entry->dimensions->width;
		uint32_t entryHeight = entry->dimensions->height;

		if( (entryWidth != width) || (entryHeight != height) )
		{
			changedDimensions = true;
			setDimensions( entryWidth, entryHeight );
		}

		// if dimensions changed at this frame, we know it's not a diff
		if( entry->header.frame == frame )
		{
			diff = false;
		}
	}

	// if this is a checkpoint, we know it's not a diff
	if( findMeta( frame,
				  PwMovieMetaEntry::CHECKPOINT,
				  false ) )
	{
		diff = false;
	}

	if( changedDimensions )
	{
		assert( !diff ); // just a sanity check
	}

	pmpdb( cout << "reading frame from offset " << ftello( file ) << endl );

	if( 0 != readrle( file, rleBuf, header.version, !diff ) )
	{
		fprintf( stderr, "readrle() failed!\n" );
		exit( 1 );
	}

	if( diff )
	{
		if( header.version >= 4 )
		{
			unrlediff4( rleBuf, rgbBuf, width, height, header.version );
		}
		else if( header.version == 3 )
		{
			unrlediff3( rleBuf, rgbBuf, width, height, header.version );
		}
		else if( header.version == 2 )
		{
			unrlediff2( rleBuf, rgbBuf, width, height, header.version );
		}
	}
	else
	{
		unrle( rleBuf, rgbBuf, width, height, header.version );
	}

	pmpdb( cout << "end of nextFrame(), width=" << width << ", height=" << height << endl );

	frame++;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---
//--- PwMovieQGLWidgetRecorder
//---
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
PwMovieQGLWidgetRecorder::PwMovieQGLWidgetRecorder( QGLWidget *widget, PwMovieWriter *writer )
{
	this->widget = widget;
	this->writer = writer;

	width = 0;
	height = 0;
	
	rgbBufOld = NULL;
	rgbBufNew = NULL;
}

PwMovieQGLWidgetRecorder::~PwMovieQGLWidgetRecorder()
{
	if( rgbBufOld ) free( rgbBufOld );
	if( rgbBufNew ) free( rgbBufNew );
}

void PwMovieQGLWidgetRecorder::recordFrame( uint32_t timestep )
{
	if( (uint32_t(widget->width()) != width) || (uint32_t(widget->height()) != height) )
	{
		setDimensions();
	}

	glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbBufNew );

	writer->writeFrame( timestep, width, height, rgbBufOld, rgbBufNew );

	uint32_t *rgbBufSwap = rgbBufNew;
	rgbBufNew = rgbBufOld;
	rgbBufOld = rgbBufSwap;
}

void PwMovieQGLWidgetRecorder::setDimensions()
{
	if( rgbBufOld ) free( rgbBufOld );
	if( rgbBufNew ) free( rgbBufNew );

	this->width = widget->width();
	this->height = widget->height();

	uint32_t rgbBufSize = width * height * sizeof(*rgbBufNew);
	rgbBufOld = (uint32_t *)malloc( rgbBufSize );
	rgbBufNew = (uint32_t *)malloc( rgbBufSize );
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---
//--- Encode/Decode Functions
//---
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void rleproc( register uint32_t *rgb,
			  register uint32_t width,
			  register uint32_t height,
			  register uint32_t *rle,
			  register uint32_t rleBufSize )
{
    register uint32_t *rgbend;
    register uint32_t n;
    register uint32_t currentrgb;
    register uint32_t len;  // does not include length (itself)
    register uint32_t *rleend;
    uint32_t *rlelen;

    rleend = rle + rleBufSize - 1;  // -1 because they come in pairs
    rlelen = rle++;  // put length at the beginning

    len = 0;
    rgbend = rgb + width*height;
    currentrgb = *rgb | AlphaMask_RGBA;

    while( (rgb < rgbend) && (rle < rleend) )
	{
        n = 1;
        while( (++rgb < rgbend) &&
			   ((*rgb | AlphaMask_RGBA) == currentrgb) )
            n++;
		pmpPrint( "encoding run of %lu pixels = %08lx\n", n, currentrgb );
        *rle++ = n;
        *rle++ = currentrgb;
        len += 2;
        if( rgb < rgbend )
            currentrgb = *rgb | AlphaMask_RGBA;
    }

    *rlelen = len;
}


void unrle( register uint32_t *rle,
			register uint32_t *rgb,
			register uint32_t width,
			register uint32_t height,
			register uint32_t version )
{
    register uint32_t *rleend;
			 uint32_t currentrgb;
    register uint32_t *rgbend;
    register uint32_t *rgbendmax;
	register uint32_t len;

	// Don't byte swap initial *rle (len), because it was already swapped in readrle()
	len = *rle;
    rleend = rle + len;  // no +1 (due to length) because they come in pairs
	//printf( "rle = %p, rleend = %p, len = %lu, rleend - rle = %lu\n", rle, rleend, len, (uint32_t)(rleend - rle) );
	rle++;
    rgbendmax = rgb + width*height;

    while( rle < rleend )
	{
		len = *rle;
	#if __BIG_ENDIAN__
		if( version > 100 )
			len = SwapInt32( len );
	#else
		if( version < 100 )
			len = SwapInt32( len );
	#endif
        rgbend = rgb + len;
		rle++;        if( rgbend > rgbendmax )
            rgbend = rgbendmax;
        currentrgb = *rle;
		rle++;
		if( version < 5 )	// from Iris, so it was ABGR, but we need RGBA, so reverse bytes
		{
			uint32_t abgr = currentrgb;
			register unsigned char* before = (unsigned char*) &abgr;
			register unsigned char* after = (unsigned char*) &currentrgb;
			for( int i = 0; i < 4; i++ )
				after[i] = before[3-i];
		}
		pmpPrint( "run of %lu pixels = %08lx\n", len, currentrgb );
        while( rgb < rgbend )
           *rgb++ = currentrgb;
    }
}


int readrle( register FILE *f, register uint32_t *rle, register uint32_t version, register bool firstFrame )
{
	register uint32_t n;
	static bool shown;	// will initialize to false

	n = fread( (char *) (rle), sizeof( *rle ), 1, f );
	
//	printf( "*rle as read = %lu (0x%08lx), *rle swapped = %u (0x%08x), version = %lu\n", *rle, *rle, SwapInt32( *rle ), SwapInt32( *rle ), version );
//	exit( 0 );
	
#if __BIG_ENDIAN__
	// If this is PPC reading Intel data, then we have to swap bytes
	if( version > 100 )
		*rle = SwapInt32( *rle );
#else
	// If this is Intel reading PPC data, then we have to swap bytes
	if( version < 100 )
		*rle = SwapInt32( *rle );
#endif
	
	if( n != 1 )
	{
		if( !shown && !feof( f ) )
		{
			fprintf( stderr, "%s: while reading length, read %u longs\n", __FUNCTION__, n);
			shown = true;
		}
		return( 1 );
	}
	if( firstFrame || (version < 4) )
	{
		n = fread( (char *) &(rle[1]), sizeof( *rle ), rle[0], f );
		pmpPrint( "read %lu longs\n", n+1 );
	}
	else
	{
		n = fread( (char *) &(rle[1]), 2, rle[0], f );
		pmpPrint( "read %lu shorts\n", n+2 );
	}

	if( n != rle[0] )
		return( 1 );

	return( 0 );
}


// rlediff2 sets the high bit to indicate changed runs

void rlediff2( register uint32_t *rgbnew,
			   register uint32_t *rgbold,
			   register uint32_t width,
			   register uint32_t height,
			   register uint32_t *rle,
			   register uint32_t rleBufSize )
{
    register uint32_t *rgbnewend;
    register uint32_t n;
    register uint32_t currentrgb;
    register uint32_t len;  // does not include length (itself)
    register uint32_t *rleend;
    uint32_t *rlelen;

    if( !rgbold )	// first time or PLAINRLE
	{
        rleproc( rgbnew, width, height, rle, rleBufSize );
        return;
    }

    rleend = rle + rleBufSize;	// no -1 because they aren't necessarily in pairs
    rlelen = rle++;	// put length at the beginning

    len = 0;
    rgbnewend = rgbnew + width*height;

    while( (rgbnew < rgbnewend) && (rle < rleend) )
	{
        // Look for unchanged pixel runs
        n = 0;
        while( (rgbnew < rgbnewend) &&
			   ( (*rgbnew | AlphaMask_RGBA) ==
				 (*rgbold | AlphaMask_RGBA) ) )
		{
            rgbnew++;
            rgbold++;
            n++;
        }
        if( n > 0 )
		{
            *rle++ = n;
            len++;
        }

        // Now that we have a difference...
        {
            // First find where they sync up again
            register uint32_t *rgbnewtmp;

            rgbnewtmp = rgbnew;

            while( (rgbnewtmp < rgbnewend) &&
				   ( (*rgbnewtmp | AlphaMask_RGBA) !=
					 (*rgbold    | AlphaMask_RGBA) ) )
			{
                rgbnewtmp++;
                rgbold++;
            }

            // Now do regular rle until they sync up
            while( (rgbnew < rgbnewtmp) && (rle < rleend-1) )	// -1 because they come in pairs here
			{
                currentrgb = *rgbnew++ | AlphaMask_RGBA;
                n = 1;
                while( (rgbnew < rgbnewtmp) &&
					   ((*rgbnew | AlphaMask_RGBA) == currentrgb) )
				{
                    rgbnew++;
                    n++;
                }
                *rle++ = n | HIGHBITON;
                *rle++ = currentrgb;
                len += 2;
            }
        }
    }
	
    *rlelen = len;
}


void unrlediff2( register uint32_t *rle,
				 register uint32_t *rgb,
				 register uint32_t width,
				 register uint32_t height,
				 register uint32_t version )
{
    register uint32_t *rleend;
			 uint32_t currentrgb;
    register uint32_t *rgbend;
    register uint32_t *rgbendmax;

    rleend = rle + *rle + 1;  // +1 because the data follows the length
	rle++;
    rgbendmax = rgb + width*height;

    while( rle < rleend )
	{
        if( *rle & HIGHBITON )	// It's a regular RLE run
		{
            if( rle < rleend-1 )	// -1 because there must be a pair of them
			{
                rgbend = rgb + (*rle++ & HIGHBITOFF);
                if( rgbend > rgbendmax )
                    rgbend = rgbendmax;
                currentrgb = *rle++;
				if( version < 5 )	// from Iris, so it was ABGR, but we need RGBA, so reverse bytes
				{
					uint32_t abgr = currentrgb;
					register unsigned char* before = (unsigned char*) &abgr;
					register unsigned char* after = (unsigned char*) &currentrgb;
					for( int i = 0; i < 4; i++ )
						after[i] = before[3-i];
				}
                while( rgb < rgbend )
                   *rgb++ = currentrgb;
            }
        }
        else  // It's a run of identical pixels
            rgb += *rle++;
    }
}


// rlediff3 is like rlediff2, but high bit is set for unchanged runs

void rlediff3( register uint32_t *rgbnew,
               register uint32_t *rgbold,
			   register uint32_t width,
			   register uint32_t height,
               register uint32_t *rle,
			   register uint32_t rleBufSize )
{
    register uint32_t *rgbnewend;
    register uint32_t n;
    register uint32_t currentrgb;
    register uint32_t len;  // does not include length (itself)
    register uint32_t *rleend;
    uint32_t *rlelen;

    if( !rgbold )	// first time or PLAINRLE
	{
        rleproc( rgbnew, width, height, rle, rleBufSize );
        return;
    }

    rleend = rle + rleBufSize;  // no -1 because they aren't necessarily in pairs
    rlelen = rle++;  // put length at the beginning

    len = 0;
    rgbnewend = rgbnew + width*height;

    while( (rgbnew < rgbnewend ) && (rle < rleend) )
	{
        // Look for unchanged pixel runs
        n = 0;
        while( (rgbnew < rgbnewend) &&
			   ((*rgbnew | AlphaMask_RGBA) ==
				(*rgbold | AlphaMask_RGBA)) )
		{
            rgbnew++;
            rgbold++;
            n++;
        }
        if( n > 0 )
		{
			*rle++ = n | HIGHBITON;
			len++;
        }

        // Now that we have a difference...
        {
            // First find where they sync up again
            register uint32_t *rgbnewtmp;

            rgbnewtmp = rgbnew;

            while( (rgbnewtmp < rgbnewend) &&
				   ((*rgbnewtmp | AlphaMask_RGBA) !=
					(*rgbold    | AlphaMask_RGBA)) )
			{
                rgbnewtmp++;
                rgbold++;
            }

            // Now do regular rle until they sync up
            while( (rgbnew < rgbnewtmp) && (rle < rleend-1) )	// -1 because they come in pairs here
			{
                currentrgb = *rgbnew++ | AlphaMask_RGBA;

                n = 1;
                while( (rgbnew < rgbnewtmp) &&
					   ((*rgbnew | AlphaMask_RGBA) == currentrgb) )
				{
                    rgbnew++;
                    n++;
                }
                *rle++ = n;
                *rle++ = currentrgb;
                len += 2;
            }
        }
    }

    *rlelen = len;
}


void unrlediff3( register uint32_t *rle,
				 register uint32_t *rgb,
				 register uint32_t width,
				 register uint32_t height,
				 register uint32_t version )
{
    register uint32_t *rleend;
			 uint32_t currentrgb;
    register uint32_t *rgbend;
    register uint32_t *rgbendmax;

    rleend = rle + *rle + 1;  // +1 because the data follows the length
	rle++;
    rgbendmax = rgb + width*height;

    while( rle < rleend )
	{
        if( *rle & HIGHBITON )	// It's a run of unchanged pixels
            rgb += *rle++ & HIGHBITOFF;
        else	// It's a regular RLE run
		{
            if( rle < rleend - 1 )	// -1 because there must be a pair of them
			{
                rgbend = rgb + *rle++;
                if( rgbend > rgbendmax )
                    rgbend = rgbendmax;
                currentrgb = *rle++;
				if( version < 5 )	// from Iris, so it was ABGR, but we need RGBA, so reverse bytes
				{
					uint32_t abgr = currentrgb;
					register unsigned char* before = (unsigned char*) &abgr;
					register unsigned char* after = (unsigned char*) &currentrgb;
					for( int i = 0; i < 4; i++ )
						after[i] = before[3-i];
				}
                while( rgb < rgbend )
                   *rgb++ = currentrgb;
            }
        }
    }
}

// rlediff4 is like rlediff3, but packs run-length into unused alpha byte

void rlediff4( register uint32_t *rgbnew,
               register uint32_t *rgbold,
			   register uint32_t width,
			   register uint32_t height,
               register uint32_t *rle,
			   register uint32_t rleBufSize )
{
    register uint32_t *rgbnewend;
    register uint32_t n;
    register uint32_t currentrgb;
    register uint32_t len;  // does not include length (itself)
    register unsigned short *srle;
    register unsigned short *srleend;

    if( !rgbold )	// first time or PLAINRLE
	{
        rleproc( rgbnew, width, height, rle, rleBufSize );
        return;
    }

    // following calculation of srleend leaves room for a long at the end
    srleend = (unsigned short *) (rle + rleBufSize);
    srle = (unsigned short *) (rle + 1);

    len = 0;
    rgbnewend = rgbnew + width*height;

    while( (rgbnew < rgbnewend) && (srle < srleend) )
	{
        // Look for unchanged pixel runs
        n = 0;
        while( (rgbnew < rgbnewend) &&
			   ((*rgbnew & NoAlphaMask_RGBA) ==
				(*rgbold & NoAlphaMask_RGBA)) )
		{
            rgbnew++;
            rgbold++;
            n++;
            if( n == (1 << 15) )	// have to save now cause we only use shorts
			{
				// Note: We encode n-1, to eek out one extra pixel, since a run of zero pixels is not possible
				pmpPrint( "unchanged run of %4ld (0x%04lx) pixels (0x%08lx) encoded as 0x%04x\n", n, n, *(rgbnew-1), (unsigned short) (n-1) | HIGHBITONSHORT );
                *srle++ = (unsigned short) (n-1) | HIGHBITONSHORT;
                len += 1;
                n = 0;
            }
        }
        if( n > 0 )
		{
			// Note: We encode n-1, to eek out one extra pixel, since a run of zero pixels is not possible
			pmpPrint( "unchanged run of %4ld (0x%04lx) pixels (0x%08lx) encoded as 0x%04x\n", n, n, *(rgbnew-1), (unsigned short) (n-1) | HIGHBITONSHORT );
            *srle++ = (unsigned short) (n-1) | HIGHBITONSHORT;
            len += 1;
        }

        // Now that we have a difference...
        {
            // First find where they sync up again
            register uint32_t *rgbnewtmp;

            rgbnewtmp = rgbnew;

            while( (rgbnewtmp < rgbnewend) &&
				   ((*rgbnewtmp & NoAlphaMask_RGBA) !=
					(*rgbold    & NoAlphaMask_RGBA)) )
			{
                rgbnewtmp++;
                rgbold++;
            }

            // Now do regular rle until they sync up
            while( (rgbnew < rgbnewtmp) && (srle < srleend) )
			{
                // no -1 is required (even though a long is), because we
                // computed srleend above so as to leave a long at the end

                currentrgb = *rgbnew++ & NoAlphaMask_RGBA;

                n = 1;
                while( (rgbnew < rgbnewtmp) &&
					   ((*rgbnew & NoAlphaMask_RGBA) == currentrgb) )
				{
                    rgbnew++;
                    n++;
                    if( n == 128 )	// have to save now cause we use 7bits+1
					{
						// Note: We encode n-1, to eek out one extra pixel, since a run of zero pixels is not possible
						pmpPrint( "  changed run of %4ld pixels (0x%08lx) encoded as 0x%04lx.%04lx\n", n, currentrgb, ((n-1) << 8) | (currentrgb & 0x000000ff), (currentrgb >> 8) & 0x0000ffff );
					#if ABGR
                        *srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 16) );
                        *srle++ = (unsigned short) (currentrgb & 0x0000ffff);
					#else
					  #if __BIG_ENDIAN__
                        *srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 24) );	// store n & r
                        *srle++ = (unsigned short) ( (currentrgb >> 8) & 0x0000ffff );		// store g & b
					  #else
                        *srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb & 0x000000ff) );	// store n & r
                        *srle++ = (unsigned short) ( (currentrgb >> 8) & 0x0000ffff );				// store g & b
					  #endif
					#endif
                        len += 2;
                        n = 0;
                    }
                }
                if( n > 0 )
				{
					// Note: We encode n-1, to eek out one extra pixel, since a run of zero pixels is not possible
					pmpPrint( "  changed run of %4ld pixels (0x%08lx) encoded as 0x%04lx.%04lx\n", n, currentrgb, ((n-1) << 8) | (currentrgb & 0x000000ff), (currentrgb >> 8) & 0x0000ffff );
				#if ABGR
					*srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 16) );
					*srle++ = (unsigned short) (currentrgb & 0x0000ffff);
				#else
				  #if __BIG_ENDIAN__
					*srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 24) );	// store n & r
					*srle++ = (unsigned short) ( (currentrgb >> 8) & 0x0000ffff );		// store g & b
				  #else
					*srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb & 0x000000ff) );	// store n & r
					*srle++ = (unsigned short) ( (currentrgb >> 8) & 0x0000ffff );				// store g & b
				  #endif
				#endif
                    len += 2;
                }
            }
        }
    }

    *rle = len;
}


void unrlediff4( register uint32_t *rle,
				 register uint32_t *rgb,
				 register uint32_t width,
				 register uint32_t height,
				 register uint32_t version )
{
			 uint32_t currentrgb;
    register uint32_t *rgbend;
    register uint32_t *rgbendmax;
    register unsigned short *srle;
    register unsigned short *srleend;

    srle = (unsigned short *) (rle + 1);
	// Don't byte swap this initial *rle (len), because it has already been swapped in readrle()
    srleend = srle + *rle;
	pmpPrint( "rle = %08lx, srle = %08lx, *rle = %lu, srleend = %08lx\n", (uint32_t) rle, (uint32_t) srle, *rle, (uint32_t) srleend );

    rgbendmax = rgb + width*height;

    while( srle < srleend )
	{
		register unsigned short len;
		
		len = *srle;
		//printf( "len = %u (0x%04x), len_swapped&off = %u (0x%04x), len>>8 = %u, len&0x00ff = %u\n",
		//		len, len, SwapInt16( len ) & HIGHBITOFFSHORT, SwapInt16( len ) & HIGHBITOFFSHORT, len>>8, len&0x00ff );

		#if __BIG_ENDIAN__
			if( version > 100 )
				len = SwapInt16( len );
		#else
			if( version < 100 )
				len = SwapInt16( len );
		#endif

        if( len & HIGHBITONSHORT )	// It's a run of unchanged pixels
		{
			len &= HIGHBITOFFSHORT;	// clear the high bit
			
			// Note: We encode n-1, to eek out one extra pixel, since a run of zero pixels is not possible
			pmpPrint( "unchanged run of %4d pixels (0x%08lx)\n", len + 1, *(rgb+1) );
            rgb += len + 1;
			srle++;
        }
        else	// It's a regular RLE run
		{
            if( srle < srleend - 1 )	// -1 so must be a long left
			{
				register unsigned short n;
				// Note: We encode n-1, to eek out one extra pixel, since a run of zero pixels is not possible
			#if __BIG_ENDIAN__
				if( version > 100 )
					n = (*srle & 0x00ff) + 1;
				else
					n = (*srle >> 8) + 1;
			#else
				if( version < 100 )
					n = (*srle & 0x00ff) + 1;
				else
					n = (*srle >> 8) + 1;
			#endif
				rgbend = rgb + n;
                if( rgbend > rgbendmax )
                    rgbend = rgbendmax;
				if( version < 5 )	// from Iris, so it was ABGR, but we need RGBA, so reverse bytes
				{
					currentrgb = AlphaMask_ABGR | ((long) (*srle) << 16) | *(srle+1);
					uint32_t abgr = currentrgb;
					register unsigned char* before = (unsigned char*) &abgr;
					register unsigned char* after = (unsigned char*) &currentrgb;
					for( int i = 0; i < 4; i++ )
						after[i] = before[3-i];
				}
				else
				{
				#if ABGR
					currentrgb = AlphaMask_ABGR | ((long) (*srle) << 16) | *(srle+1);
				#else
				  #if __BIG_ENDIAN__
					if( version < 100 )	// PPC on PPC
						currentrgb = ((long) (*srle) << 24) | ((long) *(srle+1) << 8) | AlphaMask_RGBA;
					else				// Intel on PPC
						currentrgb = ((long) (*srle & 0xff00) << 16) | ((long) *(srle+1) << 8) | AlphaMask_RGBA;
				  #else
					if( version > 100 )	// Intel on Intel
						currentrgb = (*srle & 0xff) | ((long) *(srle+1) << 8) | AlphaMask_RGBA;
					else				// PPC on Intel
						currentrgb = (*srle >> 8) | ((long) *(srle+1) << 8) | AlphaMask_RGBA;
				  #endif
				#endif
				}
				pmpPrint( "  changed run of %4d pixels (0x%08lx)\n", n, currentrgb );
                srle += 2;
                while( rgb < rgbend )
                   *rgb++ = currentrgb;
            }
        }
    }
}


char* sgets( char* string, size_t size, FILE* file )
{
	char* returnValue;
	
	returnValue = fgets( string, size, file );
	
//	printf( "%s: size = %lu, string = %s, returnValue = %p\n", __func__, size, string, returnValue );
	
	if( returnValue )
	{
		if( string[strlen(string) - 1] == '\n' )
			string[strlen(string) - 1]  = '\0';
		else
			while( fgetc( file ) != '\n' );
	}	
	
	return( returnValue );
}


double hirestime( void )
{
#ifdef linux

	struct timeval tv;
	gettimeofday( &tv, NULL );

	return (double)tv.tv_sec + (double)tv.tv_usec/1000000.0;

#else /* this seems to be Apple-specific, referencing "mach"... */

    static uint32_t num = 0;
    static uint32_t denom = 0;
    uint64_t now;

    if (denom == 0) {
            struct mach_timebase_info tbi;
            kern_return_t r;
            r = mach_timebase_info(&tbi);
            if (r != KERN_SUCCESS) {
                    abort();
            }
            num = tbi.numer;
            denom = tbi.denom;
    }
    now = mach_absolute_time();
    return (double)(now * (double)num / denom / NSEC_PER_SEC);

#endif
}


//---------------------------------------------------------------------------
// PwRecordMovie
//---------------------------------------------------------------------------
void PwRecordMovie( FILE *f, long xleft, long ybottom, long width, long height )
{
    register uint32_t n;
	// Note: We depend on the fact that the following static pointers
	// will be initialized to NULL by the linker/loader
	static uint32_t*	rgbBuf1;
	static uint32_t*	rgbBuf2;
	static uint32_t*	rleBuf;
	static uint32_t*	rgbBufNew;
	static uint32_t*	rgbBufOld;
	static uint32_t	rgbBufSize;
	static uint32_t	rleBufSize;

#ifdef PLAINRLE
	if( !rgbBufNew )	// first time
	{
		PwWriteMovieHeader( f, kCurrentMovieVersion, width, height );
		rgbBuf1 = (uint32_t*) malloc( rgbBufSize );
		rgbBufNew = rgbBuf1;
	}
#else
    if( !rgbBufNew )	// first time
	{
		PwWriteMovieHeader( f, kCurrentMovieVersion, width, height );
		rgbBufSize = width * height * sizeof( *rgbBuf1 );
		rleBufSize = rgbBufSize + 1;	// it better compress!
		rgbBuf1 = (uint32_t*) malloc( rgbBufSize );
		rgbBuf2 = (uint32_t*) malloc( rgbBufSize );
		rleBuf  = (uint32_t*) malloc( rleBufSize );
        rgbBufNew = rgbBuf1;
	}
    else	// all but the first time
	{
        register uint32_t *rgbBufsave;
        rgbBufsave = rgbBufNew;
        if (rgbBufOld)
            rgbBufNew = rgbBufOld;
        else
            rgbBufNew = rgbBuf2;
        rgbBufOld = rgbBufsave;
    }
#endif

	glReadPixels( xleft, ybottom, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbBufNew );
    
    rlediff4( rgbBufNew, rgbBufOld, width, height, rleBuf, rleBufSize );

    if( rgbBufOld )
	{  // frames 2 and on
        n = fwrite( rleBuf, 2, (int) rleBuf[0]+2, f );  // +2 to include (long) length
		pmpPrint( "wrote %lu shorts\n", n );
    }
    else
	{
        n = fwrite( rleBuf, 4, (int) rleBuf[0]+1, f );  // +1 to include (long) length
		pmpPrint( "wrote %lu longs\n", n );
    }
    
    if( n < 0 )
    	printf( "Can't happen\n" );
}

//---------------------------------------------------------------------------
// PwReadMovieHeader
//---------------------------------------------------------------------------
void PwReadMovieHeader( FILE *f, uint32_t* version, uint32_t* width, uint32_t* height )
{
	fread( (char*) version, sizeof( *version ), 1, f );
	*version = ntohl( *version );	// we always write, and therefore must read, the version as big-endian, for historical reasons (all else is native endian)
	
	fread( (char*) width,   sizeof( *width   ), 1, f );
	fread( (char*) height,  sizeof( *height  ), 1, f );
	
#if __BIG_ENDIAN__
	if( *version > 100 )
	{
		*width = SwapInt32( *width );
		*height = SwapInt32( *height );
	}
#else
	if( *version < 100 )
	{
		*width = SwapInt32( *width );
		*height = SwapInt32( *height );
	}
#endif
	
	pmpPrint( "version = %lu, width = %lu, height = %lu\n", *version, *width, *height );
}


//---------------------------------------------------------------------------
// PwWriteMovieHeader
//---------------------------------------------------------------------------
void PwWriteMovieHeader( FILE *f, uint32_t hostVersion, uint32_t width, uint32_t height )
{
	uint32_t version = htonl( hostVersion );	// we always write the version big-endian, for historical reasons (all else is native endian)
	fwrite( &version, sizeof( version ), 1, f );
	
	fwrite( &width,   sizeof( width ),   1, f );
	fwrite( &height,  sizeof( height ),  1, f );

	pmpPrint( "version = %lu, width = %lu, height = %lu\n", hostVersion, width, height );
}
