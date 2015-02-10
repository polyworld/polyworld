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
	// This gives us a reference to a per-agent opaque pointer.
	static AgentAttachedData::SlotHandle _slotHandle;
};
