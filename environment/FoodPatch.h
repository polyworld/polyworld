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

	float update();
	float addFood();
	void setInitCounts(int initFood, int minFood, int maxFood, int maxFoodGrown, float newFraction);
	void init(float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distribution, float nhsize, gstage* fStage, Domain* dm, int domainNumber);
       
};




#endif
