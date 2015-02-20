#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include "utils/ThreadPool.h"

class Scheduler
{
 public:
    typedef std::function<void()> Task;

    Scheduler();

	void execMasterTask(Task masterTask,
                        bool forceAllSerial );
	void postParallel( Task task );
	void postSerial( Task task );

 private:
    enum State {Idle, Master, Parallel, Serial} state = Idle;

    ThreadPool threadPool;

    std::vector<Task> serialTasks;
    std::mutex serialMutex;
	bool forceAllSerial;
};
