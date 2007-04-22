
// FoodPatch.cp - implementation of food patches
// Self
#include "FoodPatch.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "critter.h"
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
void FoodPatch::init(float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distrib, float nhsize, int inPeriod, float inOnFraction, float inPhase, bool inRemoveFood, gstage* fs, Domain* dm, int domainNumber){
    
	initBase(x, z,  sx, sz, shape, distrib, nhsize, fs, dm, domainNumber);

	fraction = patchFraction;
	growthRate = rate;
 	initFoodCount = initFood;
	foodCount = 0;
	foodGrown = false;

	minFoodCount = minFood;
 	maxFoodCount = maxFood;
	maxFoodGrownCount = maxFoodGrown;
	
	period = inPeriod;
	inversePeriod = 1. / period;
	onFraction = inOnFraction;
	phase = inPhase;
	removeFood = inRemoveFood;

#if DebugFoodPatches
	if( patchFraction > 0.0 )
		printf( "initing FOOD patch at (%.2f, %.2f) with size (%.2f, %.2f), frac=%g, init=%d, min=%d, max=%d, maxGrow=%d, period=%d, onFraction=%g, phase=%g, removeFood=%d\n",
				startX, startZ, sizeX, sizeZ, fraction, initFoodCount, minFoodCount, maxFoodCount, maxFoodGrownCount, period, onFraction, phase, removeFood );
	else
		printf( "initing FOOD patch at (%.2f, %.2f) with size (%.2f, %.2f), period=%d, onFraction=%g, phase=%g, removeFood=%d; limits to be determined\n",
				startX, startZ, sizeX, sizeZ, period, onFraction, phase, removeFood );
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




