//---------------------------------------------------------------------------
//	File:		brick.h
//---------------------------------------------------------------------------

#ifndef BRICK_H
#define BRICK_H

// System
#include <iostream>

using namespace std;

// Local
#include "gdlink.h"
#include "gsquare.h"
#include "BrickPatch.h"

// Forward declarations
class brick;

//===========================================================================
// brick
//===========================================================================
class brick : public gboxf
{
 public:
	static float gBrickHeight;
	static float gBrickRadius;
	static float gCarryBrick2Energy;

	static long GetNumBricks();

	BrickPatch* myBrickPatch;
	
	brick( Color color );
	brick( Color color, float x, float z );
	~brick();
    
	void dump(ostream& out);
	void load(istream& in);
    
	float pickup(float e);

	void setPatch( BrickPatch* bp );
    
 protected:
	void initBrick( Color color );
	void initBrick( Color color, float x, float z );
	void initBrick( Color color, float x, float y, float z );
	
	static unsigned long NumBricks;
	static bool BrickClassInited;
	static void InitBrickClass();

#if 0
	void setRadius();
#endif
};

inline long brick::GetNumBricks() { return NumBricks; }
inline void brick::setPatch( BrickPatch* bp ) { myBrickPatch = bp; }

#endif
