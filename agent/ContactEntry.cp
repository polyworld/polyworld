#include "ContactEntry.h"

#include <assert.h>

#include "agent.h"
#include "datalib.h"
#include "Simulation.h"

static const char *colnames[] =
	{
		"Timestep",
		"Agent1",
		"Agent2",
		"Events",
		NULL
	};

static const datalib::Type coltypes[] =
	{
		datalib::INT,
		datalib::INT,
		datalib::INT,
		datalib::STRING,
	};


void ContactEntry::start( DataLibWriter *out )
{
	out->beginTable( "Contacts",
					 colnames,
					 coltypes );
}

void ContactEntry::stop( DataLibWriter *out )
{
	out->endTable();
}


ContactEntry::ContactEntry( long _step, agent *_c, agent *_d )
{
	step = _step;
	c.init( _c );
	d.init( _d );
}

void ContactEntry::mate( agent *a, int status )
{
	get(a)->mate = status;
}

void ContactEntry::fight( agent *a, int status )
{
	get(a)->fight = status;
}

void ContactEntry::give( agent *a, int status )
{
	get(a)->give = status;
}

void ContactEntry::log( DataLibWriter *out )
{
	char buf[32];
	char *b = buf;

	c.encode( &b );
	*(b++) = 'C';
	d.encode( &b );
	*(b++) = 0;

	out->addRow( step,
				 c.number,
				 d.number,
				 buf );
}

ContactEntry::AgentInfo *ContactEntry::get( agent *a )
{
	if( a->Number() == c.number )
		return &c;
	else
		return &d;
}

void ContactEntry::AgentInfo::init( agent *a )
{
	number = a->Number();
	mate = MATE__NIL;
	fight = FIGHT__NIL;
	give = GIVE__NIL;
}

void ContactEntry::AgentInfo::encode( char **buf )
{
	char *b = *buf;

	if(mate)
	{
		*(b++) = 'M';
		
#define __SET( PREVENT_TYPE, CODE ) if( mate & MATE__PREVENTED__##PREVENT_TYPE ) *(b++) = CODE

		__SET( PARTNER, 'p' );
		__SET( CARRY, 'c' );
		__SET( MATE_WAIT, 'w' );
		__SET( ENERGY, 'e' );
		__SET( EAT_MATE_SPAN, 'f' );
		__SET( EAT_MATE_MIN_DISTANCE, 'i' );
		__SET( MAX_DOMAIN, 'd' );
		__SET( MAX_WORLD, 'x' );
		__SET( MISC, 'm' );
		__SET( MAX_VELOCITY, 'v' );

#undef __SET

#ifdef OF1
		// implement
		assert( false );
#endif
	}

	if(fight)
	{
		*(b++) = 'F';

#define __SET( PREVENT_TYPE, CODE ) if( fight & FIGHT__PREVENTED__##PREVENT_TYPE ) *(b++) = CODE

		__SET( CARRY, 'c' );
		__SET( SHIELD, 's' );
		__SET( POWER, 'p' );

#undef __SET
	}

	if(give)
	{
		*(b++) = 'G';

#define __SET( PREVENT_TYPE, CODE ) if( give & GIVE__PREVENTED__##PREVENT_TYPE ) *(b++) = CODE

		__SET( CARRY, 'c' );
		__SET( ENERGY, 'e' );

#undef __SET
	}

	*buf = b;
}
