//---------------------------------------------------------------------------
//	File:		FoodPatch.h
//---------------------------------------------------------------------------

#pragma once

//System
#include <iostream>
#include <list>

//Local
#include "cppprops.h"
#include "Energy.h"
#include "FoodType.h"
#include "gstage.h"
#include "Patch.h"

using namespace std;

// Forward declarations
class food;
class FoodPatch;

//===========================================================================
// FoodPatch
//===========================================================================
class FoodPatch : public Patch
{
	PROPLIB_CPP_PROPERTIES

 public:
	// forward decl
	class OnCondition;

	FoodPatch();
	~FoodPatch();

	food *addFood( long step );
	void setInitCounts( int initFood, int minFood, int maxFood, int maxFoodGrown, float newFraction );
	void init( const FoodType *foodType, float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distribution, float nhsize, bool on, bool inRemoveFood, gstage* fStage, Domain* dm, int domainNumber );
	bool initFoodGrown();
	void initFoodGrown( bool setInitFoodGrown );

	bool isOn();
	bool isOnChanged();
	void endStep();

	float growthRate;
    
	int foodCount;
	int initFoodCount;
	int minFoodCount;
	int maxFoodCount;
	int maxFoodGrownCount;

	float fraction;    
	float foodRate;

	bool removeFood;
	bool foodGrown;

 private:
	bool on;
	bool onPrev;
	const FoodType *foodType;
};

inline bool FoodPatch::initFoodGrown( void )
{
	return( foodGrown );
}

inline void FoodPatch::initFoodGrown( bool setInitFoodGrown )
{
	foodGrown = setInitFoodGrown;
}
