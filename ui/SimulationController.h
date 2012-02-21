#pragma once

#include <string>

#include <QObject>


//===========================================================================
// SimulationController
//===========================================================================

class SimulationController : public QObject
{
	Q_OBJECT

 public:
	SimulationController( class TSimulation *simulation );
	virtual ~SimulationController();

	class TSimulation *getSimulation();

	void start();
	std::string end( long timestep = -1 );

	bool isPaused();

 signals:
	void step();

 public slots:
	void pause();
	void resume();
	void pausedStep();

 private slots:
	void execStep();
	void simulationEnded();

 private:
	class TSimulation *simulation;
	class QTimer *timer;
	bool paused;
};
