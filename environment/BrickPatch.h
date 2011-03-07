//---------------------------------------------------------------------------
//	File:		BrickPatch.h
//---------------------------------------------------------------------------


#ifndef BRICKPATCH_H
#define BRICKPATCH_H

//System
#include <iostream>
#include "graphics.h"
#include "gstage.h"
#include "Patch.h"

using namespace std;

// Forward declarations
class BrickPatch;
class FoodPatch;

//===========================================================================
// BrickPatch
//===========================================================================
class BrickPatch : public Patch
{
 public:
	BrickPatch();
	~BrickPatch();
	
	int brickCount;

	void init(Color color, float x, float z, float sx, float sz, int numberBricks, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber, FoodPatch *onSyncFoodPatch);

	void updateOn( long step );

 private:
	void addBricks();
	void removeBricks();

	Color brickColor;
	FoodPatch *onSyncFoodPatch;
	bool isOn;
};


#endif
