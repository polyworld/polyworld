
// FoodPatch.cp - implementation of food patches
// Self
#include "FoodPatch.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "agent.h"
#include "globals.h"
#include "graphics.h"
#include "food.h"
#include "distributions.h"
#include "Simulation.h"


using namespace std;


//===========================================================================
// FoodPatch
//===========================================================================


// Default constructor
FoodPatch::FoodPatch()
{
}

//-------------------------------------------------------------------------------------------
// FoodPatch::FoodPatch
//-------------------------------------------------------------------------------------------
void FoodPatch::init(float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distrib, float nhsize, OnCondition *onCondition, bool inRemoveFood, gstage* fs, Domain* dm, int domainNumber){
    
	initBase(x, z,  sx, sz, shape, distrib, nhsize, fs, dm, domainNumber);

	fraction = patchFraction;
	growthRate = rate;
 	initFoodCount = initFood;
	foodCount = 0;
	foodGrown = false;

	minFoodCount = minFood;
 	maxFoodCount = maxFood;
	maxFoodGrownCount = maxFoodGrown;
	
	this->onCondition = onCondition;

	removeFood = inRemoveFood;

#if DebugFoodPatches
	if( patchFraction > 0.0 )
		printf( "initing FOOD patch at (%.2f, %.2f) with size (%.2f, %.2f), frac=%g, init=%d, min=%d, max=%d, maxGrow=%d, removeFood=%d\n",
				startX, startZ, sizeX, sizeZ, fraction, initFoodCount, minFoodCount, maxFoodCount, maxFoodGrownCount, removeFood );
	else
		printf( "initing FOOD patch at (%.2f, %.2f) with size (%.2f, %.2f), removeFood=%d; limits to be determined\n",
				startX, startZ, sizeX, sizeZ, removeFood );
#endif
}


// Set this patch's fraction of the world food limits
// This is used when the patch's fraction must be determined
// by its area...so these must be set after initializiation in init
void FoodPatch::setInitCounts( int initFood, int minFood, int maxFood, int maxFoodGrown, float newFraction )
{
	initFoodCount = initFood;
	minFoodCount = minFood;
	maxFoodCount = maxFood;
	maxFoodGrownCount = maxFoodGrown;
	fraction = newFraction;
#if DebugFoodPatches
	printf( "setting limits for FOOD patch at (%.2f, %.2f) with size (%.2f, %.2f) to frac=%g, init=%d, min=%d, max=%d, maxGrow=%d\n",
			startX, startZ, sizeX, sizeZ, fraction, initFoodCount, minFoodCount, maxFoodCount, maxFoodGrownCount );
#endif
}


//-------------------------------------------------------------------------------------------
// FoodPatch::~FoodPatch
//-------------------------------------------------------------------------------------------
FoodPatch::~FoodPatch()
{
	delete onCondition;
}



// Add food to the FoodPatch.
// Find an appropriate point in the patch (based on patch shape and distribution)
float FoodPatch::addFood()
{
	// Only add the food if there is room in the patch
	if( foodCount < maxFoodCount )
	{
		float x, z;
		food* f = new food;

		// set the values of x and y to a legal point in the foodpatch
		setPoint( &x, &z );
		
	#if DebugFoodPatches
		printf( "%s: adding food to patch at location (%g, %g)\n", __FUNCTION__, x, z );
	#endif
	
		f->setx( x );
		f->setz( z );

		// Set some of the Food properties
		f->domain( domainNumberOfParent );
		f->setPatch( this );

		// Finally, add it to the world object list and fStage
		objectxsortedlist::gXSortedObjects.add( f );
		fStage->AddObject( f ); 

		// Update the patch's count
		foodCount++;
		return( f->energy() );
	}
	
#if DebugFoodPatches
	printf( "%s: couldn't add food to patch because foodCount (%d) >= maxFoodCount (%d)\n", __FUNCTION__, foodCount, maxFoodCount );
#endif
	
	return( 0 );
}

//===========================================================================
// TimeOnCondition
//===========================================================================
FoodPatch::TimeOnCondition::TimeOnCondition( int _period,
											 float _onFraction,
											 float _phase )
{
	period = _period;
	inversePeriod = 1. / _period;
	onFraction = _onFraction;
	phase = _phase;
}

FoodPatch::TimeOnCondition::~TimeOnCondition()
{
}

void FoodPatch::TimeOnCondition::updateOn( long step )
{
	// no-op
}

bool FoodPatch::TimeOnCondition::on( long step )
{
	if( (period == 0) || (onFraction == 1.0) )
		return( true );
	
	float floatCycles = step * inversePeriod;
	int intCycles = (int) floatCycles;
	float cycleFraction = floatCycles  -  intCycles;
	if( (cycleFraction >= phase) && (cycleFraction < (phase + onFraction)) )
		return( true );
	else
		return( false );
}

bool FoodPatch::TimeOnCondition::turnedOff( long step )
{
	return on( step ) && !on( step + 1 );
}

//===========================================================================
// MaxPopGroupOnCondition
//===========================================================================
void FoodPatch::MaxPopGroupOnCondition::Group::validate( Group *group )
{
	if( group != NULL )
	{
		if( group->members.size() < 2 )
		{
			cerr << "MaxPopGroup must have at least 2 patches" << endl;
			exit( 1 );
		}
	}
}

void FoodPatch::MaxPopGroupOnCondition::Group::findNext( long step, MaxPopGroupOnCondition *exclude )
{
	MaxPopGroupOnCondition *minMember = NULL;

	itfor( MemberList, members, it )
	{
		MaxPopGroupOnCondition *member = *it;
		if( !exclude || (member != exclude) )
		{
			if( minMember == NULL )
			{
				minMember = member;
			}
			else
			{
				long count = member->parms.patch->agentInsideCount;
				long countMin = minMember->parms.patch->agentInsideCount;

				if( (count < countMin)
					|| ( (count == countMin) && (member->state.end < minMember->state.end) ) )
				{
					minMember = member;
				}
			}
		}
	}

	assert( minMember );

	minMember->state.start = step;

	// it's a hack to do this here. we want some kind of 'death patch' infrastructure.
	if( exclude == NULL )
	{
		FoodPatch *newPatch = minMember->parms.patch;

		// we only do killing if it wasn't a timeout, which we can tell by a null exclude
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

FoodPatch::MaxPopGroupOnCondition::MaxPopGroupOnCondition( FoodPatch *patch,
														   Group *group,
														   int maxPopulation,
														   int timeout,
														   int delay )
{
	parms.patch = patch;
	parms.group = group;
	parms.maxPopulation = maxPopulation;
	parms.timeout = timeout;
	parms.delay = delay;

	if( parms.group->members.size() == 0 )
	{
		// activate the first one
		state.start = 1;
	}
	else
	{
		state.start = -1;
	}
	state.end = -1;
	state.findNext = -1;

	parms.group->members.push_back( this );
}

FoodPatch::MaxPopGroupOnCondition::~MaxPopGroupOnCondition()
{
	parms.group->members.remove( this );
	if( parms.group->members.size() == 0 )
	{
		delete parms.group;
	}
}

void FoodPatch::MaxPopGroupOnCondition::updateOn( long step )
{
	if( state.findNext == step )
	{
		state.findNext = -1;
		parms.group->findNext( step );
	}
	else if( (state.start > -1) && (step > state.start) )
	{
		bool isOn = true;
		bool findImmediate = false;

		if( (parms.timeout > 0) && ((step - state.start) >= parms.timeout) )
		{
			isOn = false;
			findImmediate = true;
			cout << "TIMEOUT @ " << step << "(timeout=" << parms.timeout << ")" << endl;
		}
		else if( parms.patch->agentInsideCount >= parms.maxPopulation )
		{
			isOn = false;
			cout << "MAXPOP @ " << step << "(count=" << parms.patch->agentInsideCount << ")" << endl;
		}

		if( !isOn )
		{
			state.start = -1;
			state.end = step;

			if( findImmediate )
			{
				state.findNext = -1;
				parms.group->findNext( step, this );
			}
			else
			{
				state.findNext = step + parms.delay;
			}
		}
	}
}

bool FoodPatch::MaxPopGroupOnCondition::on( long step )
{
	return (state.start != -1) && (state.start <= step);
}

bool FoodPatch::MaxPopGroupOnCondition::turnedOff( long step )
{
	return state.end == step;
}
