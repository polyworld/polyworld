#include "SeparationCache.h"

#include "agent.h"
#include "datalib.h"

using namespace std;

#define __AGENT_DATA(A) (A)->external.separation_cache
#define AGENT_DATA(A) ((AgentData *)__AGENT_DATA(A))

//#define DB(X...) printf(X)
#define DB(X...)

static const char *colnames[] =
	{
		"Agent",
		"Separation",
		NULL
	};

static const datalib::Type coltypes[] =
	{
		datalib::INT,
		datalib::FLOAT
	};

// --------------------------------------------------------------------------------
// ctor()
// --------------------------------------------------------------------------------
SeparationCache::SeparationCache()
{
	log = NULL;
}

// --------------------------------------------------------------------------------
// start()
//
// Start of Simulation
// --------------------------------------------------------------------------------
void SeparationCache::start( DataLibWriter *_log )
{
	log = _log;
}

// --------------------------------------------------------------------------------
// stop()
//
// End of Simulation
// --------------------------------------------------------------------------------
void SeparationCache::stop()
{
}

// --------------------------------------------------------------------------------
// birth()
// --------------------------------------------------------------------------------
void SeparationCache::birth( agent *a )
{
	__AGENT_DATA(a) = new AgentData();
}

// --------------------------------------------------------------------------------
// death()
// --------------------------------------------------------------------------------
void SeparationCache::death( agent *a )
{
	if( log )
	{
		record( a );
	}
	delete AGENT_DATA(a);
	__AGENT_DATA(a) = NULL;
}

// --------------------------------------------------------------------------------
// separation()
// --------------------------------------------------------------------------------
float SeparationCache::separation( agent *a, agent *b )
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

	AgentData::SeparationMap &separationMap = AGENT_DATA(x)->separationMap;
	AgentData::SeparationMap::iterator it = separationMap.find( y->Number() );
	float result;
	
	if( it == separationMap.end() )
	{
		DB("  CACHE MISS\n");
		result = a->Genes()->separation( b->Genes() );
		separationMap[y->Number()] = result;
	}
	else
	{
		result = it->second;
	}

	DB("  separation=%f\n", result);

	return result;
}

// --------------------------------------------------------------------------------
// record()
// --------------------------------------------------------------------------------
void SeparationCache::record( agent *a )
{
	AgentData::SeparationMap &separationMap = AGENT_DATA(a)->separationMap;
	
	if( separationMap.size() == 0 )
	{
		return;
	}

	char buf[16];
	sprintf( buf, "%ld", a->Number() );

	log->beginTable( buf,
					 colnames,
					 coltypes );


	itfor( AgentData::SeparationMap, separationMap, it )
	{
		log->addRow( it->first, it->second );
	}

	log->endTable();
}
