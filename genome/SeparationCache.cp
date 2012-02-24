#include "SeparationCache.h"

#include "agent.h"
#include "datalib.h"

using namespace std;

//#define DB(X...) printf(X)
#define DB(X...)

AgentAttachedData::SlotHandle SeparationCache::_slotHandle;

// --------------------------------------------------------------------------------
// start()
//
// Start of Simulation
// --------------------------------------------------------------------------------
void SeparationCache::init()
{
	_slotHandle = AgentAttachedData::createSlot();
}

// --------------------------------------------------------------------------------
// birth()
// --------------------------------------------------------------------------------
void SeparationCache::birth( const sim::AgentBirthEvent &birth )
{
	AgentAttachedData::set( birth.a, _slotHandle, new AgentEntries() );
}

// --------------------------------------------------------------------------------
// death()
// --------------------------------------------------------------------------------
void SeparationCache::death( const sim::AgentDeathEvent &death )
{
	delete (AgentEntries *)AgentAttachedData::get( death.a, _slotHandle );
}

// --------------------------------------------------------------------------------
// getEntries()
// --------------------------------------------------------------------------------
SeparationCache::AgentEntries &SeparationCache::getEntries( agent *a )
{
	return *(AgentEntries *)AgentAttachedData::get( a, _slotHandle );
}


// --------------------------------------------------------------------------------
// createEntry()
// --------------------------------------------------------------------------------
float SeparationCache::createEntry( agent *a, agent *b )
{
	agent *x;
	agent *y;

	DB("SeparationCache::separation(%ld,%ld)\n", a->Number(), b->Number());

	if( a->Number() < b->Number() )
	{
		x = a;
		y = b;
	}
	else
	{
		x = b;
		y = a;
	}

	DB("  x,y=%ld,%ld\n", x->Number(), y->Number());

	AgentEntries &entries = getEntries(x);
	AgentEntries::iterator it = entries.find( y->Number() );
	float result;
	
	if( it == entries.end() )
	{
		DB("  CACHE MISS\n");
		result = a->Genes()->separation( b->Genes() );
		entries[y->Number()] = result;
	}
	else
	{
		result = it->second;
	}

	DB("  separation=%f\n", result);

	return result;
}
