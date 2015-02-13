#pragma once

#include <functional>

#include "utils/Queue.h"

class Scheduler
{
 public:
    typedef std::function<void()> Task;

	void execMasterTask(Task masterTask,
                        bool forceAllSerial );
	void postParallel( Task task );
	void postSerial( Task task );

 private:
	BusyFetchQueue<Task> parallelTasks;
	SerialQueue<Task> serialTasks;
	bool forceAllSerial;
};
