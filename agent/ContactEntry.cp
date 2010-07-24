#include "ContactEntry.h"

#include "agent.h"
#include "datalib.h"

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

void ContactEntry::mate()
{
	c.mate = true;
	d.mate = true;
}

void ContactEntry::fight( agent *a )
{
	get(a)->fight = true;
}

void ContactEntry::give( agent *a )
{
	get(a)->give = true;
}

void ContactEntry::log( DataLibWriter *out )
{
	char buf[32];
	char *b = buf;

	c.encode( &b );
	*(b++) = ',';
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
	mate = fight = give = false;
}

void ContactEntry::AgentInfo::encode( char **buf )
{
	char *b = *buf;

	if(mate)
		*(b++) = 'm';
	if(fight)
		*(b++) = 'f';
	if(give)
		*(b++) = 'g';

	*buf = b;
}
