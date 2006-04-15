// System
#include <libc.h>
#include <stdio.h>
#include <gl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdarg.h>
#include <stream.h>
#include <signal.h>
#include <math.h>

// Qt
#include <QApplication>
#include <QGLFormat>
#include <QTimer>

// Self
#include "PwMoviePlayer.h"
#include "PwMovieTools.h"
#include "MainWindow.h"

#if __BIG_ENDIAN__
char defaultMovieFileName[] = "movie.pmv";
#else
char defaultMovieFileName[] = "movie.pmv";	// "movie.pwm";
#endif

int main( int argc, char **argv )
{
//	Q_INIT_RESOURCE(application);

	// It is important the we have OpenGL support
    if (!QGLFormat::hasOpenGL())
    {
		qWarning("This system has no OpenGL support. Exiting.");
		return -1;
    }
	
	// Create application instance
	PMPApp app( argc, argv );

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
PMPApp::PMPApp(int argc, char** argv) : QApplication(argc, argv)
{
    char *movieFileName = NULL;

    if (movieFileName == NULL)
        movieFileName = defaultMovieFileName;

    mainMovieFile = fopen( movieFileName, "rb" );

    if( !mainMovieFile )
	{
        fprintf( stderr, "unable to open movie file \"%s\"\n", movieFileName );
        exit(1);
    }

	// Establish how are preference settings file will be named
	QCoreApplication::setOrganizationDomain( "indiana.edu" );
	QCoreApplication::setApplicationName( "pwmovieplayer" );
	
	mainWindow = new MainWindow( "Polyworld MoviePlayer", "Main", 0, mainMovieFile );
	mainWindow->show();
	
	// Create playback timer
	QTimer* idleTimer = new QTimer( mainWindow );
	connect( idleTimer, SIGNAL( timeout() ), this, SLOT( NextFrame() ) );
	idleTimer->start( 0 );
}


//---------------------------------------------------------------------------
// PMPApp::PMPApp
//---------------------------------------------------------------------------
PMPApp::~PMPApp()
{
	fclose( mainMovieFile );
}


//---------------------------------------------------------------------------
// PMPApp::NextFrame()
//---------------------------------------------------------------------------
void PMPApp::NextFrame()
{
	static unsigned long frame;
	frame++;
#if 0
	if( frame == 3 )
	{
		mainWindow->dumpObjectTree();
		mainWindow->menuBar()->dumpObjectTree();
	}
#endif
	
	mainWindow->NextFrame();
}
