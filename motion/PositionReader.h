#ifndef POSITION_READER_H
#define POSITION_READER_H

#include <stdio.h>

#include <map>

#include "PositionRecord.h"

//===========================================================================
// PopulationHistoryEntry
//===========================================================================
struct PopulationHistoryEntry
{
	long agent_number;
	long step_begin;
	long step_end;
};


//===========================================================================
// PopulationHistory
//===========================================================================
class PopulationHistory
{
 public:
	~PopulationHistory();

 public:
	typedef std::map<long, PopulationHistoryEntry *> AgentNumberMap;
	AgentNumberMap numberLookup;

	long step_end;
};


//===========================================================================
// PositionQuery
//===========================================================================
class PositionQuery
{
 public:
	struct StepRange
	{
		long begin;
		long end;

		inline long duration() {return end - begin + 1;}
	};

	long agent_number;
	StepRange steps_epoch;

	StepRange steps_agent;

	PositionRecord *records;
	int nrecords;

 public:
	PositionQuery() {records = NULL; nrecords = 0;}
	~PositionQuery() {if(records) delete records;}

	inline PositionRecord *get(long step) {if(step > steps_agent.end || step < steps_agent.begin) return NULL; else return records + (step - steps_epoch.begin);}
};


//===========================================================================
// PositionReader
//===========================================================================
class PositionReader
{
 public:
	PositionReader(const char *path_position_dir);
	~PositionReader();

 public:
	const PopulationHistory *getPopulationHistory();
	void getPositions(PositionQuery *query);

 private:
	void initPopulationHistory();
	FILE *open(long agent_number);

 private:
	const char *path_position_dir;
	PopulationHistory population_history;
};

#endif // POSITION_READER_H
