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
	static Color gBrickColor;

	BrickPatch* myBrickPatch;
	
	brick();
	brick( float x, float z );
	~brick();
    
	void dump(ostream& out);
	void load(istream& in);
    
	float pickup(float e);

	void setPatch( BrickPatch* bp );
    
 protected:
	void initBrick();
	void initBrick( float x, float z );
	void initBrick( float x, float y, float z );

#if 0
	void setRadius();
#endif
};

inline void brick::setPatch( BrickPatch* bp ) { myBrickPatch = bp; }

#endif
