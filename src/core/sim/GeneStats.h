#pragma once

#include "agent.h"
#include "Scheduler.h"

class GeneStats
{
 public:
	GeneStats();
	~GeneStats();

	void init( long maxAgents );

	float *getMean();
	float *getStddev();

	void compute( Scheduler &scheduler );

 private:
	long _maxAgents;
	class agent **_agents;   // list of agents to be used for computing stats.
	unsigned long *_sum;	// sum, for computing mean
	unsigned long *_sum2;	// sum of squares, for computing std. dev.
	long _nagents;
	float *_mean;
	float *_stddev;
};
