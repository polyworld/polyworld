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
	PROPLIB_CPP_PROPERTIES

 public:
	BrickPatch();
	~BrickPatch();
	
	int brickCount;

	void init( Color color, float x, float z, float sx, float sz, int numberBricks, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber, bool on );

	void updateOn();

 private:
	void addBricks();
	void removeBricks();

	Color brickColor;
	bool on;
	bool onPrev;
};


#endif
