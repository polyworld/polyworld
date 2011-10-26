#include "Scheduler.h"

void Scheduler::execMasterTask( TSimulation *sim,
								ITask &masterTask,
								bool forceAllSerial )
{
	this->sim = sim;
	this->forceAllSerial = forceAllSerial;

	if( forceAllSerial )
	{
		masterTask.task_exec( sim );
	}
	else
	{
		//************************************************************
		//************************************************************
		//************************************************************
		//*****
		//***** BEGIN PARALLEL REGION
		//*****
		//************************************************************
		//************************************************************
		//************************************************************
		parallelTasks.reset();
		serialTasks.reset();


#pragma omp parallel
		{
			//////////////////////////////////////////////////
			//// MASTER ONLY
			//////////////////////////////////////////////////
#pragma omp master
			{
				masterTask.task_exec( sim );

				parallelTasks.endOfPosts();
			}

			//////////////////////////////////////////////////
			//// MASTER & SLAVES
			//////////////////////////////////////////////////
			{
				ITask *task = NULL;

				while( parallelTasks.fetch(&task) )
				{
					task->task_exec( sim );
					delete task;
				}
			}
		}
		//************************************************************
		//************************************************************
		//************************************************************
		//*****
		//***** END PARALLEL REGION
		//*****
		//************************************************************
		//************************************************************
		//************************************************************


		// Now run all the serial tasks
		serialTasks.endOfPosts();
		{
			ITask *task = NULL;

			while( serialTasks.fetch(&task) )
			{
				task->task_exec( sim );
				delete task;
			}
		}

	}
}

void Scheduler::postParallel( ITask *task )
{
	if( forceAllSerial )
	{
		task->task_exec( sim );
		delete task;
	}
	else
	{
		parallelTasks.post( task );
	}
}

void Scheduler::postSerial( ITask *task )
{
	if( forceAllSerial )
	{
		task->task_exec( sim );
		delete task;
	}
	else
	{
		serialTasks.post( task );
	}
		
}
