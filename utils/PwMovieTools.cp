// WARNING:  rlediff2 and rlediff3 have NOT been ported to the Mac's RGBA color format.
// (They retain their original ABGR color format structure from the Iris.)
// Only rlediff4 and rleproc, the algorithms currently in use, have been updated to RGBA.
//
// However, all the unrle* routines have been modified to handle either old ABGR data,
// when the version number is 4 or less, or new RGBA data, when the version number is
// 5 or greater.

#define PMP_DEBUG 0

// TODO implement these
#define OSSwapInt32(x) (x)
#define OSSwapInt16(x) (x)

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/in.h>
#ifndef linux      /* how does one say #ifdef MAC_OSX ? */
#include <mach/mach_time.h>
#endif

#include <gl.h>

#include "PwMovieTools.h"

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
#else
	#define pmpPrint( x... )
#endif

void PwRecordMovie( FILE *f, long xleft, long ybottom, long width, long height )
{
    register unsigned long n;
	// Note: We depend on the fact that the following static pointers
	// will be initialized to NULL by the linker/loader
	static unsigned long*	rgbBuf1;
	static unsigned long*	rgbBuf2;
	static unsigned long*	rleBuf;
	static unsigned long*	rgbBufNew;
	static unsigned long*	rgbBufOld;
	static unsigned long	rgbBufSize;
	static unsigned long	rleBufSize;

#ifdef PLAINRLE
	if( !rgbBufNew )	// first time
	{
		PwWriteMovieHeader( f, kCurrentMovieVersion, width, height );
		rgbBuf1 = (unsigned long*) malloc( rgbBufSize );
		rgbBufNew = rgbBuf1;
	}
#else
    if( !rgbBufNew )	// first time
	{
		PwWriteMovieHeader( f, kCurrentMovieVersion, width, height );
		rgbBufSize = width * height * sizeof( *rgbBuf1 );
		rleBufSize = rgbBufSize + 1;	// it better compress!
		rgbBuf1 = (unsigned long*) malloc( rgbBufSize );
		rgbBuf2 = (unsigned long*) malloc( rgbBufSize );
		rleBuf  = (unsigned long*) malloc( rleBufSize );
        rgbBufNew = rgbBuf1;
	}
    else	// all but the first time
	{
        register unsigned long *rgbBufsave;
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
}


//---------------------------------------------------------------------------
// PwReadMovieHeader
//---------------------------------------------------------------------------
void PwReadMovieHeader( FILE *f, unsigned long* version, unsigned long* width, unsigned long* height )
{
	fread( (char*) version, sizeof( version ), 1, f );
	*version = ntohl( *version );	// we always write, and therefore must read, the version as big-endian, for historical reasons (all else is native endian)
	
	fread( (char*) width,   sizeof( *width   ), 1, f );
	fread( (char*) height,  sizeof( *height  ), 1, f );
	
#if __BIG_ENDIAN__
	if( *version > 100 )
	{
		*width = OSSwapInt32( *width );
		*height = OSSwapInt32( *height );
	}
#else
	if( *version < 100 )
	{
		*width = OSSwapInt32( *width );
		*height = OSSwapInt32( *height );
	}
#endif
	
	pmpPrint( "version = %lu, width = %lu, height = %lu\n", *version, *width, *height );
}


//---------------------------------------------------------------------------
// PwWriteMovieHeader
//---------------------------------------------------------------------------
void PwWriteMovieHeader( FILE *f, unsigned long hostVersion, unsigned long width, unsigned long height )
{
	uint32_t version = htonl( hostVersion );	// we always write the version big-endian, for historical reasons (all else is native endian)
	fwrite( &version, sizeof( version ), 1, f );
	
	fwrite( &width,   sizeof( width ),   1, f );
	fwrite( &height,  sizeof( height ),  1, f );

	pmpPrint( "version = %lu, width = %lu, height = %lu\n", hostVersion, width, height );
}


void rleproc( register unsigned long *rgb,
			  register unsigned long width,
			  register unsigned long height,
			  register unsigned long *rle,
			  register unsigned long rleBufSize )
{
    register unsigned long *rgbend;
    register unsigned long n;
    register unsigned long currentrgb;
    register unsigned long len;  // does not include length (itself)
    register unsigned long *rleend;
    unsigned long *rlelen;

    rleend = rle + rleBufSize - 1;  // -1 because they come in pairs
    rlelen = rle++;  // put length at the beginning

    len = 0;
    rgbend = rgb + width*height;
    currentrgb = *rgb | AlphaMask_RGBA;

    while( (rgb < rgbend) && (rle < rleend) )
	{
        n = 1;
        while( ((*++rgb | AlphaMask_RGBA) == currentrgb) &&
			   (rgb < rgbend) )
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


void unrle( register unsigned long *rle,
			register unsigned long *rgb,
			register unsigned long width,
			register unsigned long height,
			register unsigned long version )
{
    register unsigned long *rleend;
			 unsigned long currentrgb;
    register unsigned long *rgbend;
    register unsigned long *rgbendmax;
	register unsigned long len;

	// Don't byte swap initial *rle (len), because it was already swapped in readrle()
	len = *rle;
    rleend = rle + len;  // no +1 (due to length) because they come in pairs
	//printf( "rle = %p, rleend = %p, len = %lu, rleend - rle = %lu\n", rle, rleend, len, (unsigned long)(rleend - rle) );
	rle++;
    rgbendmax = rgb + width*height;

    while( rle < rleend )
	{
		len = *rle;
	#if __BIG_ENDIAN__
		if( version > 100 )
			len = OSSwapInt32( len );
	#else
		if( version < 100 )
			len = OSSwapInt32( len );
	#endif
        rgbend = rgb + len;
		rle++;
        if( rgbend > rgbendmax )
            rgbend = rgbendmax;
        currentrgb = *rle;
		rle++;
		if( version < 5 )	// from Iris, so it was ABGR, but we need RGBA, so reverse bytes
		{
			unsigned long abgr = currentrgb;
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


int readrle( register FILE *f, register unsigned long *rle, register unsigned long version, register bool firstFrame )
{
	register unsigned long n;
	static bool shown;	// will initialize to false

	n = fread( (char *) (rle), sizeof( *rle ), 1, f );
	
//	printf( "*rle as read = %lu (0x%08lx), *rle swapped = %u (0x%08x), version = %lu\n", *rle, *rle, OSSwapInt32( *rle ), OSSwapInt32( *rle ), version );
//	exit( 0 );
	
#if __BIG_ENDIAN__
	// If this is PPC reading Intel data, then we have to swap bytes
	if( version > 100 )
		*rle = OSSwapInt32( *rle );
#else
	// If this is Intel reading PPC data, then we have to swap bytes
	if( version < 100 )
		*rle = OSSwapInt32( *rle );
#endif
	
	if( n != 1 )
	{
		if( !shown && !feof( f ) )
		{
			fprintf( stderr, "%s: while reading length, read %lu longs\n", __FUNCTION__, n);
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

void rlediff2( register unsigned long *rgbnew,
			   register unsigned long *rgbold,
			   register unsigned long width,
			   register unsigned long height,
			   register unsigned long *rle,
			   register unsigned long rleBufSize )
{
    register unsigned long *rgbnewend;
    register unsigned long n;
    register unsigned long currentrgb;
    register unsigned long len;  // does not include length (itself)
    register unsigned long *rleend;
    unsigned long *rlelen;

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
            register unsigned long *rgbnewtmp;

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


void unrlediff2( register unsigned long *rle,
				 register unsigned long *rgb,
				 register unsigned long width,
				 register unsigned long height,
				 register unsigned long version )
{
    register unsigned long *rleend;
			 unsigned long currentrgb;
    register unsigned long *rgbend;
    register unsigned long *rgbendmax;

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
					unsigned long abgr = currentrgb;
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

void rlediff3( register unsigned long *rgbnew,
               register unsigned long *rgbold,
			   register unsigned long width,
			   register unsigned long height,
               register unsigned long *rle,
			   register unsigned long rleBufSize )
{
    register unsigned long *rgbnewend;
    register unsigned long n;
    register unsigned long currentrgb;
    register unsigned long len;  // does not include length (itself)
    register unsigned long *rleend;
    unsigned long *rlelen;

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
            register unsigned long *rgbnewtmp;

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


void unrlediff3( register unsigned long *rle,
				 register unsigned long *rgb,
				 register unsigned long width,
				 register unsigned long height,
				 register unsigned long version )
{
    register unsigned long *rleend;
			 unsigned long currentrgb;
    register unsigned long *rgbend;
    register unsigned long *rgbendmax;

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
					unsigned long abgr = currentrgb;
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

void rlediff4( register unsigned long *rgbnew,
               register unsigned long *rgbold,
			   register unsigned long width,
			   register unsigned long height,
               register unsigned long *rle,
			   register unsigned long rleBufSize )
{
    register unsigned long *rgbnewend;
    register unsigned long n;
    register unsigned long currentrgb;
    register unsigned long len;  // does not include length (itself)
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
            register unsigned long *rgbnewtmp;

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


void unrlediff4( register unsigned long *rle,
				 register unsigned long *rgb,
				 register unsigned long width,
				 register unsigned long height,
				 register unsigned long version )
{
			 unsigned long currentrgb;
    register unsigned long *rgbend;
    register unsigned long *rgbendmax;
    register unsigned short *srle;
    register unsigned short *srleend;

    srle = (unsigned short *) (rle + 1);
	// Don't byte swap this initial *rle (len), because it has already been swapped in readrle()
    srleend = srle + *rle;
	pmpPrint( "rle = %08lx, srle = %08lx, *rle = %lu, srleend = %08lx\n", (unsigned long) rle, (unsigned long) srle, *rle, (unsigned long) srleend );

    rgbendmax = rgb + width*height;

    while( srle < srleend )
	{
		register unsigned short len;
		
		len = *srle;
		//printf( "len = %u (0x%04x), len_swapped&off = %u (0x%04x), len>>8 = %u, len&0x00ff = %u\n",
		//		len, len, OSSwapInt16( len ) & HIGHBITOFFSHORT, OSSwapInt16( len ) & HIGHBITOFFSHORT, len>>8, len&0x00ff );

		#if __BIG_ENDIAN__
			if( version > 100 )
				len = OSSwapInt16( len );
		#else
			if( version < 100 )
				len = OSSwapInt16( len );
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
					unsigned long abgr = currentrgb;
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


