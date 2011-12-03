#pragma once

#include <stdlib.h>
#include <map>

#include "misc.h"


// --- Definition ---


#pragma pack( push, 1 )

struct _AgentEvent
{
	bool eat : 1;
	bool mate : 1;
};
typedef struct _AgentEvent AgentEvent;

#pragma pack( pop )

typedef std::map <long, AgentEvent> AgentEventsMapType;

class Events
{
  public:
  	Events( long simMaxSteps );
  	~Events();
  	
  	void AddEvent( long step, long agentNumber, char event );
  	AgentEvent GetAgentEvent( long step, long agentNumber );
  	AgentEventsMapType GetAgentEventsMap( long step );
  
  private:
  	long maxSteps;
	AgentEventsMapType* events;
};


// --- Implementation ---

inline Events::Events( long simMaxSteps )
{
	maxSteps = simMaxSteps;
	events = new AgentEventsMapType[maxSteps+1];	// +1 due to steps counting from one
}

inline Events::~Events()
{
	for( long step = 0; step <= maxSteps; step++ )
		events[step].clear();
	delete[] events;
}

inline void Events::AddEvent( long step, long agentNumber, char event )
{
	if( event == 'e' )
		events[step][agentNumber].eat = true;
	else if( event == 'm' )
		events[step][agentNumber].mate = true;
}

inline AgentEvent Events::GetAgentEvent( long step, long agentNumber )
{
	if( events[step].find( agentNumber ) == events[step].end() )
	{
		AgentEvent agentEvent;
		agentEvent.eat = false;
		agentEvent.mate = false;
		return( agentEvent );
	}
	else
		return( events[step][agentNumber] );
}

inline AgentEventsMapType Events::GetAgentEventsMap( long step )
{
	return( events[step] );
}
