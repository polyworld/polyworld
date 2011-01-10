#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "PwMovieUtils.h"

using namespace std;


void usage( string msg = "" )
{
	cerr << "usage: pmvutil clip path_input startFrame endFrame path_output " << endl;

	if( msg.length() > 0 )
	{
		cerr << "--------------------------------------------------------------------------------" << endl;
		cerr << msg << endl;
	}

	exit( 1 );
}

void clip( const char *pathInput, uint32_t frameStart, uint32_t frameEnd, const char *pathOutput );

int main( int argc, char **argv )
{
	if( argc < 2 )
	{
		usage( "Must specify mode" );
	}

	string mode = argv[1];

	if( mode == "clip" )
	{
		if( argc != 6 )
		{
			usage();
		}

		clip( argv[2], (uint32_t)atol(argv[3]), (uint32_t)atol(argv[4]), argv[5] );
	}

	return 0;
}

void clip( const char *pathInput, uint32_t frameStart, uint32_t frameEnd, const char *pathOutput )
{
	FILE *fileInput = fopen( pathInput, "r" );
	if( !fileInput )
		usage( string("Cannot open input file '") + pathInput + "'" );

	if( frameStart < 1 )
		usage( "startFrame must be >= 1" );
	if( frameEnd < 1 )
		usage( "endFrame must be >= 1" );
	if( frameStart > frameEnd )
		usage( "startFrame > frameEnd" );


	PwMovieReader *reader = new PwMovieReader( fileInput );

	if( frameEnd > reader->getFrameCount() )
	{
		char msg[128];
		sprintf( msg, "endFrame (%u) greater than size of input movie (%u)", frameEnd, reader->getFrameCount() );
		usage( msg );
	}

	FILE *fileOutput = fopen( pathOutput, "w" );
	if( !fileOutput )
		usage( string("Cannot open output file '") + pathOutput + "'" );

	PwMovieWriter *writer = new PwMovieWriter( fileOutput );
	
	uint32_t oldwidth = 0;
	uint32_t oldheight = 0;
	uint32_t *rgbBufNew = NULL;
	uint32_t *rgbBufOld = NULL;

	for( uint32_t frame = frameStart; frame <= frameEnd; frame++ )
	{
		uint32_t timestep;
		uint32_t width;
		uint32_t height;
		uint32_t *rgbBuf;

		reader->readFrame( frame,
						   &timestep,
						   &width,
						   &height,
						   &rgbBuf );

		if( (width != oldwidth) || (height != oldheight) )
		{
			if( rgbBufOld ) delete rgbBufOld;
			if( rgbBufNew ) delete rgbBufNew;

			rgbBufOld = new uint32_t[ width * height ];
			rgbBufNew = new uint32_t[ width * height ];

			oldwidth = width;
			oldheight = height;
		}

		memcpy( rgbBufNew, rgbBuf, sizeof(uint32_t) * width * height );

		writer->writeFrame( timestep,
							width,
							height,
							rgbBufOld,
							rgbBufNew );

		uint32_t *rgbBufSwap = rgbBufNew;
		rgbBufNew = rgbBufOld;
		rgbBufOld = rgbBufSwap;
	}

	writer->close();

	delete writer;
	delete reader;
}
