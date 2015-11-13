// System
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// STL
#include <string>

// Qt
#include <qgl.h>
#include <QApplication>

// Local
#include "monitor/Monitor.h"
#include "monitor/MonitorManager.h"
#include "proplib/proplib.h"
#include "sim/Simulation.h"
#include "ui/SimulationController.h"
#include "ui/gui/MainWindow.h"
#include "ui/term/TerminalUI.h"


using namespace std;

//===========================================================================
// usage
//===========================================================================
void usage( const char* format, ... )
{
	printf( "Usage:  Polyworld [--ui gui|term] worldfile\n" );

	if( format )
	{
		printf( "Error:\n\t" );
		va_list argv;
		va_start( argv, format );
		vprintf( format, argv );
		va_end( argv );
		printf( "\n" );
	}

	exit( 1 );
}

void usage()
{
	usage( NULL );
}

//===========================================================================
// main
//===========================================================================
int main( int argc, char** argv )
{
	const char *worldfilePath = NULL;
	string ui = "gui";
	proplib::ParameterMap parameters;

	for( int argi = 1; argi < argc; argi++ )
	{
		string arg = argv[argi];

		if( arg[0] == '-' )	// it's a flagged argument
		{
			if( arg[1] != '-' )
			{
				usage( "Unknown argument: %s", arg.c_str() );
			}
			string key = arg.substr(2);
			if( ++argi >= argc )
				usage( "Missing %s arg", arg.c_str() );
			string value( argv[argi] );
			if( key == "ui" )
				ui = value;
			else
				parameters[key] = value;
		}
		else
		{
			if( worldfilePath == NULL )
				worldfilePath = argv[argi];
			else
				usage( "Only one worldfile path allowed, at least two specified (%s, %s)", worldfilePath, argv[argi] );
		}
	}

	if( (ui != "gui") && (ui != "term") )
	{
		usage( "Invalid --ui arg (%s)", ui.c_str() );
	}

	if( ! worldfilePath )
	{
		usage( "A valid path to a worldfile must be specified" );
	}

	string monitorPath;
	{
		if( exists("./" + ui + ".mf") )
			monitorPath = "./" + ui + ".mf";
		else
			monitorPath = "./etc/" + ui + ".mf";
	}

	// Make sure we're in an appropriate working directory
#if __linux__
	{
		char exe[1024];
		int rc = readlink( "/proc/self/exe", exe, sizeof(exe) );
		exe[rc] = 0;
		char *lastslash = strrchr( exe, '/' );
		*lastslash = 0;
		if( 0 != strcmp(exe, getenv("PWD")) )
		{
			fprintf( stderr, "Must execute from directory containing binary: %s\n", exe );
			exit( 1 );
		}
	}
#endif

	QApplication app(argc, argv);

    if (!QGLFormat::hasOpenGL())
    {
		qWarning("This system has no OpenGL support. Exiting.");
		return -1;
    }

	// Establish how our preference settings file will be named
	QCoreApplication::setOrganizationDomain( "indiana.edu" );
	QCoreApplication::setApplicationName( "polyworld" );

	// It is necessary to force the "C" locale because Qt adopts the system
	// locale, but we want floats specified with a period to work correctly
	// even in locales that normally use a comma for the decimal mark.
	setlocale( LC_NUMERIC, "C" );

	proplib::Interpreter::init();

	TSimulation *simulation = new TSimulation( worldfilePath, parameters );
    MonitorManager *monitorManager = new MonitorManager(simulation, monitorPath);
	SimulationController *simulationController = new SimulationController( simulation,
                                                                           monitorManager);

    proplib::Interpreter::dispose();

	int exitval;
    function<void()> dispose_ui;

	if( ui == "gui" )
	{
		MainWindow *mainWindow = new MainWindow( simulationController );
        dispose_ui = [mainWindow]() {delete mainWindow;};
	}
	else if( ui == "term" )
	{
		TerminalUI *term = new TerminalUI( simulationController );
		dispose_ui = [term]() {delete term;};
	}
	else
		assert( false );

    simulationController->start();
    exitval = app.exec();

    dispose_ui();

	delete simulationController;
	delete simulation;
    delete monitorManager;

	return exitval;
}
