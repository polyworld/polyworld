#include "Scheduler.h"

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
				masterTask();

				parallelTasks.endOfPosts();
			}

			//////////////////////////////////////////////////
			//// MASTER & SLAVES
			//////////////////////////////////////////////////
			{
				Task task;

				while( parallelTasks.fetch(&task) )
				{
					task();
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
			Task task;

			while( serialTasks.fetch(&task) )
			{
				task();
			}
		}

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
		parallelTasks.post( task );
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
		serialTasks.post( task );
	}
		
}
