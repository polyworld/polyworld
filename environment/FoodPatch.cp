
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
void FoodPatch::init( const FoodType *foodType, float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distrib, float nhsize, bool on, bool inRemoveFood, gstage* fs, Domain* dm, int domainNumber ){
    
	initBase(x, z,  sx, sz, shape, distrib, nhsize, fs, dm, domainNumber);

	fraction = patchFraction;
	growthRate = rate;
 	initFoodCount = initFood;
	foodCount = 0;
	foodGrown = false;

	minFoodCount = minFood;
 	maxFoodCount = maxFood;
	maxFoodGrownCount = maxFoodGrown;
	
	this->on = on;
	this->onPrev = false;
	this->foodType = foodType;

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
}



//-------------------------------------------------------------------------------------------
// FoodPatch::addFood
//
// Add food to the FoodPatch.
// Find an appropriate point in the patch (based on patch shape and distribution)
//-------------------------------------------------------------------------------------------
food *FoodPatch::addFood( long step )
{
	// Only add the food if there is room in the patch
	if( foodCount < maxFoodCount )
	{
		float x, z;
		food* f = new food( foodType, step );

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
		return f;
	}
	
#if DebugFoodPatches
	printf( "%s: couldn't add food to patch because foodCount (%d) >= maxFoodCount (%d)\n", __FUNCTION__, foodCount, maxFoodCount );
#endif
	
	return NULL;
}

//-------------------------------------------------------------------------------------------
// FoodPatch::isOn
//-------------------------------------------------------------------------------------------
bool FoodPatch::isOn()
{
	return on;
}

//-------------------------------------------------------------------------------------------
// FoodPatch::isOnChanged
//
// Note that changes would come from a dynamic property in the worldfile.
//-------------------------------------------------------------------------------------------
bool FoodPatch::isOnChanged()
{
	return on != onPrev;
}

//-------------------------------------------------------------------------------------------
// FoodPatch::endStep
//-------------------------------------------------------------------------------------------
void FoodPatch::endStep()
{
	onPrev = on;
}
