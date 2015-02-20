#pragma once

#include <list>
#include <vector>

#include "complexity_algorithm.h"

class MotionComplexity
{
 public:
	struct Agent
	{
		long number;
		long birth;

		long begin;
		long end;
	};
	typedef std::list<Agent> AgentList;

	struct Epoch
	{
		int index;
		long begin;
		long end;
		AgentList agents;

		struct Complexity
		{
			float value;
			long nagents;
			float nil_ratio;
		} complexity;
	};
	typedef std::vector<Epoch> EpochList;

 public:
	MotionComplexity( const char *path_run,
					  long step_begin,
					  long step_end,
					  long epoch_len,
					  float min_epoch_presence );
	~MotionComplexity();

	float calculate( Epoch &epoch );

 public:	
	EpochList epochs;

 private:
	void computeEpochs(	long begin,
						long end,
						long epochlen );
	float getPresence( Agent &agent,
					   Epoch &epoch );
	gsl_matrix *populateMatrix( Epoch &epoch );

 private:
	const char *path_run;
	float min_epoch_presence;
};
