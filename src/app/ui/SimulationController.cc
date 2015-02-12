#include "SimulationController.h"

#include <QApplication>
#include <QTimer>

#include "sim/Simulation.h"
#include "monitor/MonitorManager.h"

//===========================================================================
// SimulationController
//===========================================================================

//---------------------------------------------------------------------------
// SimulationController::SimulationController
//---------------------------------------------------------------------------
SimulationController::SimulationController( TSimulation *simulation_,
                                            MonitorManager *monitorManager_ )
	: simulation( simulation_ )
    , monitorManager( monitorManager_ )
	, timer( new QTimer(this) )
	, paused( false )
{
	connect(timer, SIGNAL(timeout()), this, SLOT(execStep()));

    simulation->stepEnding += [=]{monitorManager->step();};
    simulation->ended += [=](){simulationEnded();};
}

//---------------------------------------------------------------------------
// SimulationController::~SimulationController
//---------------------------------------------------------------------------
SimulationController::~SimulationController()
{
}

//---------------------------------------------------------------------------
// SimulationController::getSimulation
//---------------------------------------------------------------------------
TSimulation *SimulationController::getSimulation()
{
	return simulation;
}

//---------------------------------------------------------------------------
// SimulationController::getMonitorManager
//---------------------------------------------------------------------------
MonitorManager *SimulationController::getMonitorManager()
{
	return monitorManager;
}

//---------------------------------------------------------------------------
// SimulationController::start
//---------------------------------------------------------------------------
void SimulationController::start()
{
	// Start the simulation
	timer->start( 0 );
}

//---------------------------------------------------------------------------
// SimulationController::end
//---------------------------------------------------------------------------
string SimulationController::end( long timestep )
{
	if( timestep != -1 )
	{
		return simulation->EndAt( timestep );
	}
	else
	{
		simulation->End( "userExit" );
		return "";
	}
}

//---------------------------------------------------------------------------
// SimulationController::isPaused
//---------------------------------------------------------------------------
bool SimulationController::isPaused()
{
	return paused;
}

//---------------------------------------------------------------------------
// SimulationController::pause
//---------------------------------------------------------------------------
void SimulationController::pause()
{
	if( !paused )
	{
		timer->stop();
		timer->setSingleShot( true );
		paused = true;
	}
}

//---------------------------------------------------------------------------
// SimulationController::resume
//---------------------------------------------------------------------------
void SimulationController::resume()
{
	if( paused )
	{
		timer->setSingleShot( false );
		timer->start( 0 );
		paused = false;
	}
}

//---------------------------------------------------------------------------
// SimulationController::pausedStep
//---------------------------------------------------------------------------
void SimulationController::pausedStep()
{
	if( paused )
	{
		timer->start();
	}
}

//---------------------------------------------------------------------------
// SimulationController::execStep
//---------------------------------------------------------------------------
void SimulationController::execStep()
{
	emit step();

	simulation->Step();
}

//---------------------------------------------------------------------------
// SimulationController::simulationEnded
//---------------------------------------------------------------------------
void SimulationController::simulationEnded()
{
	QCoreApplication::exit( 0 );
}
