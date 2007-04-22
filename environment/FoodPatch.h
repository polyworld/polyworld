//---------------------------------------------------------------------------
//	File:		FoodPatch.h
//---------------------------------------------------------------------------


#ifndef FOODPATCH_H
#define FOODPATCH_H

//System
#include <iostream>
#include "gstage.h"
#include "Patch.h"

using namespace std;

// Forward declarations
class FoodPatch;

//===========================================================================
// FoodPatch
//===========================================================================
class FoodPatch : public Patch
{
 public:
	FoodPatch();
	~FoodPatch();

	float growthRate;
    
	int foodCount;
	int initFoodCount;
	int minFoodCount;
	int maxFoodCount;
	int maxFoodGrownCount;

	float fraction;    
	float foodRate;
	
	int period;
	float inversePeriod;
	float onFraction;
	float phase;
	bool removeFood;
	
	bool foodGrown;

	float update();
	float addFood();
	void setInitCounts( int initFood, int minFood, int maxFood, int maxFoodGrown, float newFraction );
	void init( float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distribution, float nhsize, float inPeriod, float inOnFraction, float inPhase, bool inRemoveFood, gstage* fStage, Domain* dm, int domainNumber );
	bool on( long step );
	bool initFoodGrown();
	void initFoodGrown( bool setInitFoodGrown );
};

inline bool FoodPatch::on( long step )
{
	if( (period == 0) || (onFraction == 1.0) )
		return( true );
	
	float floatCycles = step * inversePeriod;
	int intCycles = int(floatCycles);
	float cycleFraction = floatCycles  -  intCycles;
	if( (cycleFraction >= phase) && (cycleFraction < (phase + onFraction)) )
		return( true );
	else
		return( false );
}

inline bool FoodPatch::initFoodGrown( void )
{
	return( foodGrown );
}

inline void FoodPatch::initFoodGrown( bool setInitFoodGrown )
{
	foodGrown = setInitFoodGrown;
}


#endif
