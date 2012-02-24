#pragma once

#include <map>

#include "AgentAttachedData.h"
#include "simtypes.h"

class SeparationCache
{
 private:
	SeparationCache() {}

 public:
	static void init();

	static void birth( const sim::AgentBirthEvent &birth );
	static void death( const sim::AgentDeathEvent &death );

	static float createEntry( agent *a, agent *b );

	typedef std::map<long, float> AgentEntries;
	static AgentEntries &getEntries( agent *a );

 private:
	static AgentAttachedData::SlotHandle _slotHandle;
};
