#include "simtypes.h"

#include "agent.h"
#include "simconst.h"

using namespace sim;


//===========================================================================
// AgentContactBeginEvent
//===========================================================================
AgentContactBeginEvent::AgentContactBeginEvent( agent *_c, agent *_d )
{
	c.init( _c );
	d.init( _d );
}

void AgentContactBeginEvent::mate( agent *a, int status ) { get(a)->mate = status; }
void AgentContactBeginEvent::fight( agent *a, int status ) { get(a)->fight = status; }
void AgentContactBeginEvent::give( agent *a, int status ) { get(a)->give = status; }

AgentContactBeginEvent::AgentInfo *AgentContactBeginEvent::get( agent *a ) { return a == c.a ? &c : &d; }

void AgentContactBeginEvent::AgentInfo::init( agent *_a )
{
	a = _a;
	number = _a->Number();
	mate = MATE__NIL;
	fight = FIGHT__NIL;
	give = GIVE__NIL;
}


//===========================================================================
// AgentContactEndEvent
//===========================================================================
AgentContactEndEvent::AgentContactEndEvent( const AgentContactBeginEvent &e )
{
	c.init( e.c );
	d.init( e.d );
}

void AgentContactEndEvent::AgentInfo::init( const AgentContactBeginEvent::AgentInfo &begin )
{
	number = begin.number;
	mate = begin.mate;
	fight = begin.fight;
	give = begin.give;
}
