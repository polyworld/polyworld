#include "state.h"

#include "Simulation.h"

using namespace std;
using namespace proplib;


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS __StateObject
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
CppProperties::UpdateContext *__StateObject::getUpdateContext()
{
	return CppProperties::_context;
}

long __StateObject::getStep()
{
	return CppProperties::_context->sim->getStep();
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Periodic
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Periodic::Periodic( int period,
					float onFraction,
					float phase )
: _period( period )
, _inversePeriod( 1. / period )
, _onFraction( onFraction )
, _phase( phase )
{
	assert( period >= 0 );
	assert( onFraction >= 0 && onFraction <= 1 );
	assert( phase >= 0 && phase <= 1 );
}

Periodic::~Periodic()
{
}

bool Periodic::update()
{
	if( (_period == 0) || (_onFraction == 1.0) )
		return( true );

	long step = getStep();
	
	float floatCycles = step * _inversePeriod;
	int intCycles = (int) floatCycles;
	float cycleFraction = floatCycles  -  intCycles;
	if( (cycleFraction >= _phase) && (cycleFraction < (_phase + _onFraction)) )
		return( true );
	else
		return( false );
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS FoodPatchTokenRing
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

int FoodPatchTokenRing::_maxPopulation = -1;
int FoodPatchTokenRing::_timeout = -1;
int FoodPatchTokenRing::_delay = -1;
long FoodPatchTokenRing::_step = -1;
long FoodPatchTokenRing::_delayEnd = -1;
vector<FoodPatchTokenRing::Member *> FoodPatchTokenRing::_members;
FoodPatchTokenRing::Member *FoodPatchTokenRing::_active = NULL;

void FoodPatchTokenRing::add( FoodPatch &patch, int maxPopulation, int timeout, int delay )
{
	if( maxPopulation != -1 )
	{
		assert( (_maxPopulation == -1) || (_maxPopulation == maxPopulation) );
		_maxPopulation = maxPopulation;
	}
	if( timeout != -1 )
	{
		assert( (_timeout == -1) || (_timeout == timeout) );
		_timeout = timeout;
	}
	if( delay != -1 )
	{
		assert( (_delay == -1) || (_delay == delay) );
		_delay = delay;
	}

	Member *member = new Member();
	member->patch = &patch;
	member->start = -1;
	member->end = -1;

	_members.push_back( member );
}

bool FoodPatchTokenRing::update( FoodPatch &patch )
{
	if( _step != getStep() )
	{
		if( _step == 1 )
		{
			assert( (_maxPopulation > 0) || (_timeout > 0) );
			assert( _members.size() > 1 );
		}

		updateActive();
	}

	return _active && _active->patch ==  &patch;
}

void FoodPatchTokenRing::updateActive()
{
	_step = getStep();

	if( _step == 1 )
	{
		findActive( false, NULL );
	}
	else if( _step == _delayEnd )
	{
		_delayEnd = -1;
		findActive( true, NULL );
	}
	else if( _active )
	{
		bool find = false;
		bool findImmediate;
		bool killAgents;

		if( (_timeout > 0) && ((_step - _active->start) >= _timeout) )
		{
			find = true;
			findImmediate = true;
			killAgents = false;
		}
		else if( (_maxPopulation > 0) && (_active->patch->agentInsideCount >= _maxPopulation) )
		{
			find = true;
			findImmediate = false;
			killAgents = true;
		}

		if( find )
		{
			_active->start = -1;
			_active->end = _step;

			if( (_delay <= 0) || findImmediate )
			{
				_delayEnd = -1;
				findActive( killAgents, _active );
			}
			else
			{
				_delayEnd = _step + _delay;
				_active = NULL;
			}
		}
	}
}

void FoodPatchTokenRing::findActive( bool killAgents, Member *exclude )
{
	Member *minMember = NULL;

	itfor( vector<Member *>, _members, it )
	{
		Member *member = *it;
		if( !exclude || (member != exclude) )
		{
			if( minMember == NULL )
				minMember = member;
			else
			{
				long count = member->patch->agentInsideCount;
				long countMin = minMember->patch->agentInsideCount;

				if( (count < countMin)
					|| ( (count == countMin) && (member->end < minMember->end) ) )
				{
					minMember = member;
				}
			}
		}
	}

	assert( minMember );

	_active = minMember;
	_active->start = _step;

	// Kill agents in new patch.
	if( killAgents )
	{
		FoodPatch *newPatch = _active->patch;

		agent *a;
		objectxsortedlist::gXSortedObjects.reset();
		while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&a))
		{
			if( newPatch->pointIsInside( a->x(), a->z(), 5 ) )
			{
				a->SetDeathByPatch();
			}
		}
	}

}
