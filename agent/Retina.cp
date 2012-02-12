#include "Retina.h"

#include <gl.h>
#include <stdio.h>
#include <stdlib.h>

#include "AbstractFile.h"
#include "Brain.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"
#include "Simulation.h"

// SlowVision, if turned on, will cause the vision neurons to slowly
// integrate what is rendered in front of them into their input neural activation.
// They will do so at a rate defined by TauVision, adjusting color like this:
//     new = TauVision * image  +  (1.0 - TauVision) * old
// SlowVision is traditionally off, and TauVision will initially be 0.2.
#define SlowVision false
#define TauVision 0.2


Retina::Retina( int width )
{
	this->width = width;
	
	buf = (unsigned char *)calloc( width * 4, sizeof(unsigned char) );

#if PrintBrain
	bprinted = false;
#endif
}

Retina::~Retina()
{
	free( buf );
}

void Retina::sensor_grow( NervousSystem *cns )
{
	channels[0].init( this, cns, 0, "red" );
	channels[1].init( this, cns, 1, "green" );
	channels[2].init( this, cns, 2, "blue" );
}

void Retina::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	for( int i = 0; i < width * 4; i++ )
	{
		buf[i] = (unsigned char)(rng->range(0.0, 255.0));
	}

	sensor_update( false );
}

void Retina::sensor_update( bool bprint )
{
	IF_BPRINTED
	(
		printf( "numneurons red,green,blue=%d, %d, %d\n",
				channels[0].numneurons, channels[1].numneurons, channels[2].numneurons );
		printf( "xwidth red,green,blue=%g, %g, %g\n",
				channels[0].xwidth, channels[1].xwidth, channels[2].xwidth );
		printf( "xintwidth red,green,blue=%d, %d, %d,\n",
				channels[0].xintwidth, channels[1].xintwidth, channels[2].xintwidth );
	)

	for( int i = 0; i < 3; i++ )
	{
		channels[i].update( bprint );
	}

	IF_BPRINT
	(
        printf("***** step = %ld ****** overheadrank = %d ******\n", TSimulation::fStep, TSimulation::fOverHeadRank);
        printf("retinaBuf [0 - %d]\n",(brain::retinawidth - 1));
        printf("red:");
        
        for( int i = 0; i < (brain::retinawidth * 4); i+=4 )
            printf(" %3d", buf[i]);
        printf("\ngreen:");
        
        for( int i = 1; i < (brain::retinawidth * 4); i+=4 )
            printf(" %3d",buf[i]);
        printf("\nblue:");
        
        for( int i = 2; i < (brain::retinawidth * 4); i+=4 )
            printf(" %3d", buf[i]);
        printf("\n");		
	)
}

void Retina::sensor_start_functional( AbstractFile *f )
{
	for( int i = 0; i < 3; i++ )
	{
		channels[i].start_functional( f );
	}
}

void Retina::sensor_dump_anatomical( AbstractFile *f )
{
	for( int i = 0; i < 3; i++ )
	{
		channels[i].dump_anatomical( f );
	}
}

void Retina::updateBuffer( short x, short y,
						   short width, short height )
{
	// The POV window must be the current GL context,
	// when agent::UpdateVision is called, for both the
	// DrawAgentPOV() call above and the glReadPixels()
	// call below.  It is set in TSimulation::Step().
			
	glReadPixels(x,
				 y + height / 2,
				 width,
				 1,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 buf);

	debugcheck( "after glReadPixels" );

#if 0
	static FILE* pixelFile = NULL;
	if( pixelFile == NULL )
	{
		pixelFile = fopen( "run/pixels.txt", "w");
		if( !pixelFile )
		{
			fprintf( stderr, "Unable to open pixels.txt\n" );
			exit( 1 );
		}
		fprintf( pixelFile, "retina pixels:\n" );
	}
	
	for( int i = 0; i < width; i++ )
	{
		for( int j = 0; j < 4; j++ )
			fprintf( pixelFile, "%02x", buf[i*4 + j] );
	}
	fprintf( pixelFile, "\n" );
#endif
}

const unsigned char *Retina::getBuffer()
{
	return buf;
}

void Retina::Channel::init( Retina *retina,
							NervousSystem *cns,
							int index,
							const char *name )
{
	this->name = name;
	this->index = index;
	buf = retina->buf;

	nerve = cns->get( name );

	numneurons = nerve->getNeuronCount();

	int width = retina->width;

	xwidth = float(width) / numneurons;
	xintwidth = width / numneurons;

	if( (xintwidth * numneurons) != width )
	{
		xintwidth = 0;
	}
}

void Retina::Channel::update( bool bprint )
{
    short pixel;
    float avgcolor;
    float endpixloc;
    
    if( xintwidth )
    {
        pixel = 0;
        for(int i = 0; i < numneurons; i++)
        {
            avgcolor = 0.0;
            for (short ipix = 0; ipix < xintwidth; ipix++)
                avgcolor += buf[((pixel++) * 4) + index];

			nerve->set( i, avgcolor / (xwidth * 255.0) );
        }
    }
    else
    {
        pixel = 0;
        avgcolor = 0.0;

		BPRINT("x%swidth = %f\n", name, xwidth);

        for (int i = 0; i < numneurons; i++)
        {
            endpixloc = xwidth * float(i+1);

			BPRINT("  neuron %d, endpixloc = %g\n", i, endpixloc);

            while (float(pixel) < (endpixloc - 1.0))
            {
                avgcolor += buf[((pixel++) * 4) + index];
				BPRINT("    in loop with pixel %d, avgcolor = %g\n", pixel,avgcolor);
                IF_BPRINT
				(
					if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc - 1.0 - 1.0e-5)))
						BPRINT("Got in-loop borderline case\n");
                )
            }
            
            avgcolor += (endpixloc - float(pixel)) * buf[(pixel * 4) + index];
			nerve->set( i, avgcolor / (xwidth * 255.0) );

			BPRINT("    after loop with pixel %d, avgcolor = %g, color = %g\n", pixel, avgcolor, nerve->get(i));
            IF_BPRINT
			(
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0 + 1.0e-5)))
                    printf("Got outside-loop borderline case\n");
			)
			avgcolor = (1.0 - (endpixloc - float(pixel))) * buf[(pixel * 4) + index];
			BPRINT("  before incrementing pixel = %d, avgcolor = %g\n", pixel, avgcolor);

            pixel++;
        }
    }

#if SlowVision
	for(int i = 0; i < numneurons; i++ )
	{
		float prev = nerve->get( i, Nerve::SWAP );
		float curr = nerve->get( i, Nerve::CURRENT );

		nerve->set( i,
				  TauVision * curr  +  (1.0 - TauVision) * prev );
	}
#endif
}

void Retina::Channel::start_functional( AbstractFile *f )
{
	int i = nerve->getIndex();

	f->printf( " %d-%d", i, i + numneurons - 1 );
}

void Retina::Channel::dump_anatomical( AbstractFile *f )
{
	int i = nerve->getIndex();

	f->printf( " %sinput=%d-%d", nerve->name.c_str(), i, i + numneurons - 1 );
}
