#pragma once

#include "Queue.h"

// forward decl
class TSimulation;

class ITask
{
 public:
	virtual ~ITask() {}

	virtual void task_exec( TSimulation *sim ) = 0;
};

class Scheduler
{
 public:
	void execMasterTask( TSimulation *sim,
						 ITask &masterTask,
						 bool forceAllSerial );
	void postParallel( ITask *task );
	void postSerial( ITask *task );

 private:
	BusyFetchQueue<ITask *> parallelTasks;
	SerialQueue<ITask *> serialTasks;
	bool forceAllSerial;
	TSimulation *sim;
};
