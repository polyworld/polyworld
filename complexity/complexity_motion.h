#ifndef COMPLEXITY_MOTION_H
#define COMPLEXITY_MOTION_H

struct CalcComplexity_motion_parms
{
	const char *path;
	long step_begin;
	long step_end;
	long epoch_len;
	float min_epoch_presence;
};

class CalcComplexity_motion_result
{
 public:
	CalcComplexity_motion_parms parms_requested;
	CalcComplexity_motion_parms parms;
 
	int ndimensions;

	class Epochs
	{
	public:
		~Epochs()
		{
	  		delete complexity;
	  		delete agent_count;
	  		delete agent_count_calculated;
			delete nil_count;
		}

		int count;

		double *complexity;
		int *agent_count;
		int *agent_count_calculated;
		int *nil_count;
	} epochs;
};

class CalcComplexity_motion_callback
{
 public:
	virtual ~CalcComplexity_motion_callback() {}

	virtual void begin(CalcComplexity_motion_parms &parms) = 0;
	virtual void epoch_result(CalcComplexity_motion_result *result,
				  int epoch_index) = 0;
	virtual void end(CalcComplexity_motion_result *result) = 0;
};

CalcComplexity_motion_result *CalcComplexity_motion(CalcComplexity_motion_parms &parms,
						    CalcComplexity_motion_callback *callback = 0);


#ifdef COMPLEXITY_MOTION_CP

#include <list>
#include <map>

#include "PositionReader.h"

class PopulateMatrixContext
{
 public:
	PopulateMatrixContext(CalcComplexity_motion_result *result);
	~PopulateMatrixContext();

	void computeEpochs();

	CalcComplexity_motion_result *result;

	PositionReader position_reader;
	PositionQuery query;

	long step;

	typedef std::map<long, const PopulationHistoryEntry *> EntryList;

	class Epoch
	{
	public:
	  EntryList entries;
	};

	Epoch *epochs;
	int nepochs;
};

#endif // COMPLEXITY_MOTION_CP

#endif // COMPLEXITY_MOTION_H
