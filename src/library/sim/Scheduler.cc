#include "Scheduler.h"

#include <assert.h>
#include <iostream>
#include <thread>

using namespace std;

static unsigned get_thread_count()
{
    unsigned ncores = thread::hardware_concurrency();
    if(ncores == 0)
    {
        ncores = 1;
        cerr << "Unable to determine CPU core count via thread::hardware_concurrency(), assuming 1 core." << endl;
    }
    return ncores - 1; // (ncores - 1) helper threads in thread pool + 1 master thread
                       // occupies CPU.
}

Scheduler::Scheduler()
    : threadPool(get_thread_count())
{
}

void Scheduler::execMasterTask( Task masterTask,
								bool forceAllSerial )
{
	this->forceAllSerial = forceAllSerial;

	if( forceAllSerial )
	{
		masterTask();
	}
	else
	{
        assert(state == Idle);
        state = Master;

        masterTask();

        state = Parallel;
        threadPool.join();

        state = Serial;
        for(Task &task: serialTasks)
        {
            task();
        }
        serialTasks.clear();

        state = Idle;
	}
}

void Scheduler::postParallel( Task task )
{
	if( forceAllSerial )
	{
		task();
	}
	else
	{
        assert(state == Master);
        threadPool.schedule( task );
	}
}

void Scheduler::postSerial( Task task )
{
	if( forceAllSerial )
	{
		task();
	}
	else
	{
        lock_guard<mutex> lock(serialMutex);
        assert(state == Master);

        serialTasks.push_back(task);
	}
		
}
