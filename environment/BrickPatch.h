//---------------------------------------------------------------------------
//	File:		BrickPatch.h
//---------------------------------------------------------------------------


#ifndef BRICKPATCH_H
#define BRICKPATCH_H

//System
#include <iostream>
#include "gstage.h"
#include "Patch.h"

using namespace std;

// Forward declarations
class BrickPatch;


//===========================================================================
// BrickPatch
//===========================================================================
class BrickPatch : public Patch
{
 public:
	BrickPatch();
	~BrickPatch();
	
	int brickCount;

	void addBrick();
	void init(float x, float z, float sx, float sz, int numberBricks, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber);
    
};


#endif
