// WARNING:  rlediff2 and rlediff3 have NOT been ported to the Mac's RGBA color format.
// (They retain their original ABGR color format structure from the Iris.)
// Only rlediff4 and rleproc, the algorithms currently in use, have been updated to RGBA.
//
// However, all the unrle* routines have been modified to handle either old ABGR data,
// when the version number is 4 or less, or new RGBA data, when the version number is
// 5 or greater.

#define PMP_DEBUG 0

#include <stdlib.h>

#include <gl.h>

#include "PwMovieTools.h"

#define HIGHBITON 0x80000000
#define HIGHBITOFF 0x7fffffff
#define HIGHBITONSHORT 0x8000
#define HIGHBITOFFSHORT 0x7fff
#define AlphaMask_ABGR 0xff000000
#define AlphaMask_RGBA 0x000000ff
#define NoAlphaMask_ABGR 0x00ffffff
#define NoAlphaMask_RGBA 0xffffff00

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
	fread( (char*) version, sizeof( *version ), 1, f );
	fread( (char*) width,   sizeof( *width   ), 1, f );
	fread( (char*) height,  sizeof( *height  ), 1, f );
	
	pmpPrint( "version = %lu, width = %lu, height = %lu\n", *version, *width, *height );
}


//---------------------------------------------------------------------------
// PwWriteMovieHeader
//---------------------------------------------------------------------------
void PwWriteMovieHeader( FILE *f, unsigned long version, unsigned long width, unsigned long height )
{
	fwrite( &version, sizeof( version ), 1, f );
	fwrite( &width,   sizeof( width ),   1, f );
	fwrite( &height,  sizeof( height ),  1, f );

	pmpPrint( "version = %lu, width = %lu, height = %lu\n", version, width, height );
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

    rleend = rle + *rle;  // no +1 (due to length) because they come in pairs
	rle++;
    rgbendmax = rgb + width*height;

    while( rle < rleend )
	{
	#if PMP_DEBUG
		unsigned long len = *rle;
	#endif
        rgbend = rgb + *rle++;
        if( rgbend > rgbendmax )
            rgbend = rgbendmax;
        currentrgb = *rle++;
		pmpPrint( "run of %lu pixels = %08lx\n", len, currentrgb );
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


int readrle( register FILE *f, register unsigned long *rle, register unsigned long version )
{
	register unsigned long n;
	static bool shown;	// will initialize to false

	n = fread( (char *) (rle), sizeof( *rle ), 1, f );
	if( n != 1 )
	{
		if( !shown )
		{
			fprintf( stderr, "%s: while reading length, read %lu longs\n", __FUNCTION__, n);
			shown = true;
		}
		return( 1 );
	}
	if( version < 4 )
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
                *srle++ = (unsigned short) (n-1) | HIGHBITONSHORT;
                len += 1;
                n = 0;
            }
        }
        if( n > 0 )
		{
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
					#if ABGR
                        *srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 16) );
                        *srle++ = (unsigned short) (currentrgb & 0x0000ffff);
					#else
                        *srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 24) );	// store n & r
                        *srle++ = (unsigned short) ( (currentrgb >> 8) & 0x0000ffff );		// store g & b
					#endif
                        len += 2;
                        n = 0;
                    }
                }
                if( n > 0 )
				{
				#if ABGR
					*srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 16) );
					*srle++ = (unsigned short) (currentrgb & 0x0000ffff);
				#else
					*srle++ = (unsigned short) ( ((n-1) << 8) | (currentrgb >> 24) );	// store n & r
					*srle++ = (unsigned short) ( (currentrgb >> 8) & 0x0000ffff );		// store g & b
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
    srleend = srle + *rle;
	pmpPrint( "rle = %08lx, srle = %08lx, *rle = %lu, srleend = %08lx\n", (unsigned long) rle, (unsigned long) srle, *rle, (unsigned long) srleend );

    rgbendmax = rgb + width*height;

    while( srle < srleend )
	{
        if( *srle & HIGHBITONSHORT )	// It's a run of unchanged pixels
		{
			pmpPrint( "unchanged run of %4d pixels\n", (*srle & HIGHBITOFFSHORT) + 1 );
            rgb += (*srle++ & HIGHBITOFFSHORT) + 1;
        }
        else	// It's a regular RLE run
		{
            if( srle < srleend - 1 )	// -1 so must be a long left
			{
                rgbend = rgb + (*srle >> 8) + 1;
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
					currentrgb = ((long) (*srle) << 24) | ((long) *(srle+1) << 8) | AlphaMask_RGBA;
				#endif
				}
				pmpPrint( "  changed run of %4d pixels (0x%08lx)\n", (*srle >> 8) + 1, currentrgb );
                srle += 2;
                while( rgb < rgbend )
                   *rgb++ = currentrgb;
            }
        }
    }
}


