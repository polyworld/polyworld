#include "EatStatistics.h"

#include <assert.h>

#include <iostream>
#include <limits>

using namespace std;

#define EAT_STATS_AVERAGE_STEPS 100
#define EAT_STATS_AVERAGE_MIN_ATTEMPTS 1

#if 1
#define TRACE(stmt)
#else
#define TRACE(stmt) stmt
#endif

void EatStatistics::Init()
{
	step.numAttempts = 0;
	step.numFailed = 0;
	step.numFailedYaw = 0;
	step.numFailedVel = 0;
	average.numAttemptsList.clear();
	average.numFailedList.clear();
	average.numFailedYawList.clear();
	average.numFailedVelList.clear();
	average.numAttempts = 0;
	average.numFailed = 0;
	average.numFailedYaw = 0;
	average.numFailedVel = 0;
	average.ratioFailed = numeric_limits<float>::quiet_NaN();
	average.ratioFailedYaw = numeric_limits<float>::quiet_NaN();
	average.ratioFailedVel = numeric_limits<float>::quiet_NaN();
}

void EatStatistics::StepBegin()
{
	step.numAttempts = 0;
	step.numFailed = 0;
	step.numFailedYaw = 0;
	step.numFailedVel = 0;
}

void EatStatistics::StepEnd()
{
	TRACE( cout << "numAttempts=" << step.numAttempts << endl );

	if( step.numAttempts >= EAT_STATS_AVERAGE_MIN_ATTEMPTS )
	{
		TRACE( cout << "sizeof numatt=" << average.numAttemptsList.size() << endl );

		if( average.numAttemptsList.size() >= EAT_STATS_AVERAGE_STEPS )
		{
			long dropped;

			dropped = average.numAttemptsList.front();
			average.numAttempts -= dropped;

			dropped = average.numFailedList.front();
			average.numFailed -= dropped;

			dropped = average.numFailedYawList.front();
			average.numFailedYaw -= dropped;

			dropped = average.numFailedVelList.front();
			average.numFailedVel -= dropped;

			average.numAttemptsList.pop_front();
			average.numFailedList.pop_front();
			average.numFailedYawList.pop_front();
			average.numFailedVelList.pop_front();
		}

		average.numAttemptsList.push_back( step.numAttempts );
		average.numFailedList.push_back( step.numFailed );
		average.numFailedYawList.push_back( step.numFailedYaw );
		average.numFailedVelList.push_back( step.numFailedVel );

		average.numAttempts += step.numAttempts;
		average.numFailed += step.numFailed;
		average.numFailedYaw += step.numFailedYaw;
		average.numFailedVel += step.numFailedVel;
		average.ratioFailed = float(average.numFailed) / average.numAttempts;
		average.ratioFailedYaw = float(average.numFailedYaw) / average.numAttempts;
		average.ratioFailedVel = float(average.numFailedVel) / average.numAttempts;

		TRACE( cout << "ratio failed Step=" << float(step.numFailed) / step.numAttempts << endl );
		TRACE( cout << "ratio failed Avg=" << average.ratioFailed << endl );
		TRACE( cout << "ratio failed Yaw Avg=" << average.ratioFailedYaw << endl );
		TRACE( cout << "ratio failed Vel Avg=" << average.ratioFailedVel << endl );
	} 
}

void EatStatistics::AgentEatAttempt( bool success, bool failedYaw, bool failedVel, bool failedMinAge )
{
	step.numAttempts++;

	if( !success )
	{
		step.numFailed++;
		if( failedYaw )
			step.numFailedYaw++;
		if( failedVel )
			step.numFailedVel++;
	}
}

const float *EatStatistics::GetProperty( const string &name )
{
	if( name == "EatFailed" )
	{
		return &average.ratioFailed;
	}
	else if( name == "EatFailedYaw" )
	{
		return &average.ratioFailedYaw;
	}
	else if( name == "EatFailedVel" )
	{
		return &average.ratioFailedVel;
	}
	else
	{
		assert( false );
		return NULL;
	}
}
