//---------------------------------------------------------------------------
//	File:		brick.h
//---------------------------------------------------------------------------

//#ifndef BRICK_H
//#define BRICK_H

// System
#include <iostream>

using namespace std;

// Local
#include "gdlink.h"
#include "gsquare.h"

// Forward declarations
//class brick;



//===========================================================================
// brick
//===========================================================================
class brick : public gboxf
{
 public:
	static float gBrickHeight;
	static Color gBrickColor;
	static float gMinBrickEnergy;
	static float gMaxBrickEnergy;

	float requiredEnergy;
	
	brick();
	brick(float e);
	brick(float e, float x, float z);
	~brick();
    
	void dump(ostream& out);
	void load(istream& in);
    
	float pickup(float e);
    
 protected:
	void initbrick();
	void initbrick(float e);
	void initbrick(float e, float x, float z);
	void initpos();
	void initlen();
	void initrest();
   	virtual void setradius();
	
};




