#include "TerminalUI.h"

#include <QApplication>
#include <QTimer>

#include "misc.h"
#include "MainWindow.h"
#include "Monitor.h"
#include "MonitorManager.h"
#include "Prompt.h"
#include "Simulation.h"
#include "SimulationController.h"

using namespace std;


//===========================================================================
// TerminalUI
//===========================================================================

//---------------------------------------------------------------------------
// TerminalUI::TerminalUI
//---------------------------------------------------------------------------
TerminalUI::TerminalUI( SimulationController *_simulationController )
	: simulationController( _simulationController )
	, statusTextMonitor( NULL )
	, gui( NULL )
{
	connectMonitors();

	prompt = new Prompt();

	connect( simulationController, SIGNAL(step()),
			 this, SLOT(step()) );
}

//---------------------------------------------------------------------------
// TerminalUI::~TerminalUI
//---------------------------------------------------------------------------
TerminalUI::~TerminalUI()
{
	if( gui )
		delete gui;
}

//---------------------------------------------------------------------------
// TerminalUI::connectMonitors
//---------------------------------------------------------------------------
void TerminalUI::connectMonitors()
{
	citfor( Monitors, simulationController->getSimulation()->getMonitorManager()->getMonitors(), it )
	{
		Monitor *monitor = *it;

		if( monitor->getType() == Monitor::STATUS_TEXT )
		{
			statusTextMonitor = dynamic_cast<StatusTextMonitor *>( monitor );
			connect( statusTextMonitor, SIGNAL(update()),
					 this, SLOT(status()) );
		}
	}
}

//---------------------------------------------------------------------------
// TerminalUI::step
//---------------------------------------------------------------------------
void TerminalUI::step()
{
	char *_cmd = prompt->getUserInput();
	if( _cmd )
	{
		string cmd = _cmd;
		free( _cmd );

		if( cmd == "end" )
		{
			simulationController->end();
		}
		else if( cmd == "gui" )
		{
			if( gui == NULL )
			{
				gui = new MainWindow( simulationController, false );
				connect( gui, SIGNAL(closing()),
						 this, SLOT(guiClosing()) );
				cout << "GUI shown. You may need to raise the window." << endl;
			}
		}
		else
		{
			if( cmd != "help" )
			{
				cerr << "Invalid command." << endl;
			}

			cerr << "help - Show this message." << endl;
			cerr << "end - End simulation." << endl;
			cerr << "gui - Show GUI." << endl;
		}
	}
}

//---------------------------------------------------------------------------
// TerminalUI::status
//---------------------------------------------------------------------------
void TerminalUI::status()
{
	if( prompt->isActive() )
		return;

	printf( "------------------------------------------------------------\n" );

	itfor( StatusText, statusTextMonitor->getStatusText(), it )
	{
		printf( "%s\n", *it );
	}
}

//---------------------------------------------------------------------------
// TerminalUI::guiClosing
//---------------------------------------------------------------------------
void TerminalUI::guiClosing()
{
	delete gui;
	gui = NULL;
}
