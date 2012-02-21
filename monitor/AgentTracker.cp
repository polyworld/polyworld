#include "AgentTracker.h"

#include "agent.h"

using namespace std;

AgentTracker::AgentTracker( string _name, const Parms &_parms )
	: listener( this )
	, name( _name )
	, parms( _parms )
{
	target = NULL;
}

AgentTracker::~AgentTracker()
{
}

const char *AgentTracker::getName()
{
	return name.c_str();
}

agent *AgentTracker::getTarget()
{
	return target;
}

const AgentTracker::Parms &AgentTracker::getParms()
{
	return parms;
}

string AgentTracker::getStateTitle()
{
	char buf[128] = "";

	if( target )
	{
		const char *prefix = parms.trackTilDeath ? "T" : "";
	
		switch( parms.mode )
		{
		case FITNESS:
			sprintf( buf, "%s%d:%ld", prefix, parms.fitness.rank, target->Number() );
			break;
		default:
			assert(false);
		}
	}
	else
	{
		strcpy( buf, "No Agent" );
	}

	return buf;
}

void AgentTracker::setTarget( agent *a )
{
	if( a != target )
	{
		if( target )
			target->removeListener( &listener );

		target = a;

		if( target )
			target->addListener( &listener );

		emit targetChanged( this );
	}
}

void AgentTracker::Listener::died( agent *a )
{
	tracker->setTarget( NULL );
}
