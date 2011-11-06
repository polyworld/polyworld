#pragma once

#include <list>
#include <string>

class EatStatistics
{
 public:
	void Init();
	void StepBegin();
	void StepEnd();
	void AgentEatAttempt( bool success, bool failedYaw, bool failedVel, bool failedMinAge );

	const float *GetProperty( const std::string &name );

 private:
	struct Step
	{
		long numAttempts;
		long numFailed;
		long numFailedYaw;
		long numFailedVel;
	} step;

	struct Average
	{
		std::list<long> numAttemptsList;
		std::list<long> numFailedList;
		std::list<long> numFailedYawList;
		std::list<long> numFailedVelList;
		long numAttempts;
		long numFailed;
		long numFailedYaw;
		long numFailedVel;
		float ratioFailed;
		float ratioFailedYaw;
		float ratioFailedVel;
	} average;
};
