// System
//#include <libc.h>
#include <stdio.h>
#include <gl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>

// Qt
#include <QApplication>
#include <QGLFormat>

// Self
#include "PwMoviePlayer.h"
#include "PwMovieUtils.h"
#include "MainWindow.h"

#define MaxLegendLines 10
#define MaxLegendLength 80

char defaultMovieFileName[] = "run/movie.pmv";

int main( int argc, char **argv )
{
	/*
	{
		PwMovieReader *reader = new PwMovieReader( fopen("run/movie.pmv", "r") );

		uint32_t timestep;
		uint32_t width;
		uint32_t height;
		uint32_t *rgbBuf;

		for(uint32_t i = 1; i <= Mreader->getFrameCount(); i++)
		{
			reader->readFrame( i, &timestep, &width, &height, &rgbBuf );
		}

		return 0;
	}
	*/

//	Q_INIT_RESOURCE(application);

	// Create application instance.
	// Moved this call above hasOpenGL() call due to Linux requirement
	// of PMPApp being created prior to call (CMB 3/7/08)
	PMPApp app( argc, argv );

	// It is important the we have OpenGL support
    if (!QGLFormat::hasOpenGL())
    {
		qWarning("This system has no OpenGL support. Exiting.");
		return -1;
    }

#if 0
	// Just confirms behavior of shift and mask on big-endian and little-endian machines is the same
	for( int i = 0; i < 1000; i += 16 )
		printf( "i = %4d = 0x%08x\n", i, i );
	for( int i = 0; i < 32; i++ )
		printf( "i = %4d, 1 << i = %11d = 0x%08x\n", i, 1<<i, 1<<i );
	for( int i = 0; i < 32; i++ )
		printf( "i = %4d, 1 >> i = %11d = 0x%08x\n", i, 1>>i, 1>>i );
	for( int i = 0; i < 32; i += 8 )
	{
		int test = 1 << i;
		printf( "i = %4d, test = 1 << i = %11d = 0x%08x, test & 0x000000ff = 0x%08x, test & 0x0000ff00 = 0x%08x, test & 0x00ff0000 = 0x%08x, test & 0xff000000 = 0x%08x\n", i, test, test, test & 0x000000ff, test & 0x0000ff00, test & 0x00ff0000, test & 0xff000000 );
	}
	exit( 0 );
#endif

	return( app.exec() );
};

//===========================================================================
// PMPApp
//===========================================================================

//---------------------------------------------------------------------------
// PMPApp::PMPApp
//---------------------------------------------------------------------------
PMPApp::PMPApp(int &argc, char** argv) : QApplication(argc, argv)
{
    char*			movieFileName	= NULL;
	char*			legendFileName	= NULL;
	char**			legend			= NULL;
	uint32_t		startFrame		= 0;
	uint32_t		endFrame		= 0;
	double			frameRate		= 75.0;
	bool			loop			= false;
	int				arg				= 1;

	while( arg < argc )
	{
		if( argv[arg][0] == '-' )
		{
			switch( argv[arg][1] )
			{
				case 'f':
					arg++;
					movieFileName = (char*) malloc( strlen( argv[arg] ) + 1 );
					if( movieFileName )
						strcpy( movieFileName, argv[arg] );
					else
						fprintf( stderr, "Unable to allocate %lu bytes of memory for movieFileName\n", strlen( argv[arg] ) + 1 );
					break;

				case 'l':
					arg++;
					legendFileName = (char*) malloc( strlen( argv[arg] ) + 1 );
					if( legendFileName )
					{
						strcpy( legendFileName, argv[arg] );
//						printf( "legendFileName = %s\n", legendFileName );
					}
					else
						fprintf( stderr, "Unable to allocate %lu bytes of memory for legendFileName\n", strlen( argv[arg] ) + 1 );
					break;

				case 's':
					arg++;
					startFrame = strtoul( argv[arg], NULL, 10 );
					break;

				case 'e':
					arg++;
					endFrame = strtoul( argv[arg], NULL, 10 );
//					printf( "endFrame = %lu\n", endFrame );
					break;

				case 'r':
					arg++;
					frameRate = atof( argv[arg] );
					break;

				case 'c':
					arg++;
					loop = true;
					break;

				default:
					fprintf( stderr, "Unknown argument -%c\n", argv[arg][1] );
					break;

			}
		}
		else
		{
			fprintf( stderr, "Unknown argument %s\n", argv[arg] );
		}
		arg++;
	}

    if (movieFileName == NULL)
        movieFileName = defaultMovieFileName;

    mainMovieFile = fopen( movieFileName, "rb" );
	if( !mainMovieFile )
	{
		fprintf( stderr, "Failed opening %s\n", movieFileName );
		::exit( 1 );
	}
	reader = new PwMovieReader( mainMovieFile );

    if( mainMovieFile )
	{
		// read text for legend from legendFileName
		if( legendFileName )
		{
			FILE* legendFile = fopen( legendFileName, "r" );

			if( legendFile )
			{
				legend = (char**) malloc( (MaxLegendLines+1) * sizeof( char* ) );	// +1 to have NULL terminating pointer

				if( legend )
				{
					int i;

					for( i = 0; i < MaxLegendLines; i++ )
					{
						char* test;

						legend[i] = (char*) malloc( MaxLegendLength + 1 );	// +1 for null terminator
						if( legend[i] )
						{
							test = sgets( legend[i], MaxLegendLength+1, legendFile );
							if( !test )
							{
								free( legend[i] );
								break;
							}
//							printf( "legend[%d] = %s\n", i, legend[i] );
						}
						else
						{
							fprintf( stderr, "Unable to allocate %d bytes of memory for legend[%d]\n", MaxLegendLength + 1, i );
							break;
						}
					}
					legend[i] = NULL;
				}
				else
				{
					fprintf( stderr, "Unable to allocate %lu bytes of memory for legend\n", (MaxLegendLines+1) * sizeof( char* ) );
				}
			}
			else
			{
				fprintf( stderr, "Unable to open legend file \"%s\"\n", legendFileName );
			}
		}
	}
	else
	{
        fprintf( stderr, "unable to open movie file \"%s\"\n", movieFileName );
    }

	// Establish how our preference settings file will be named
	QCoreApplication::setOrganizationDomain( "indiana.edu" );
	QCoreApplication::setApplicationName( "pwmovieplayer" );

	mainWindow = new MainWindow( "Polyworld MoviePlayer", "Main", 0, reader, legend, startFrame, endFrame, frameRate, loop );
	mainWindow->show();
}


//---------------------------------------------------------------------------
// PMPApp::PMPApp
//---------------------------------------------------------------------------
PMPApp::~PMPApp()
{
	if( mainMovieFile )
		fclose( mainMovieFile );
}


//---------------------------------------------------------------------------
// PMPApp::NextFrame()
//---------------------------------------------------------------------------
void PMPApp::NextFrame()
{
	/*
	static uint32_t frame;

	if( !mainMovieFile )
		exit( 0 );

	frame++;
#if 0
	if( frame == 3 )
	{
		mainWindow->dumpObjectTree();
		mainWindow->menuBar()->dumpObjectTree();
	}
#endif

	mainWindow->NextFrame();
	*/
}
