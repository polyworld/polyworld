//---------------------------------------------------------------------------
//	File:		food.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef FOOD_H
#define FOOD_H

// System
#include <iostream>

using namespace std;

// Local
#include "gdlink.h"
#include "gsquare.h"
//#include "ObjectList.h"

#include "FoodPatch.h"

// Forward declarations
class food;


//===========================================================================
// food
//===========================================================================
class food : public gboxf
{
public:
	static float gFoodHeight;
	static Color gFoodColor;
	static float gMinFoodEnergy;
	static float gMaxFoodEnergy;
	static float gSize2Energy; // (converts between food/agent size and available energy)
	static float gMaxFoodRadius;
	static float gCarryFood2Energy;

    food();
    food(float e);
    food(float e, float x, float z);
    ~food();
    
	void dump(ostream& out);
	void load(istream& in);
    
	float eat(float e);
    
	float energy();
	
	void setenergy(float e);

	void setPatch(FoodPatch* fp);
	FoodPatch* getPatch();

	short domain();
	void domain(short id);

protected:
    void initfood();
    void initfood(float e);
    void initfood(float e, float x, float z);
	void initlen();
	void initrest();
   	virtual void setradius();
	
    static unsigned long fFoodEver;

    float fEnergy;
    short fDomain;

	FoodPatch* patch; // pointer to this food's patch

};

//===========================================================================
// inlines
//===========================================================================
inline float food::energy() { return fEnergy; }
inline void food::setPatch(FoodPatch* fp) { patch=fp; }
inline FoodPatch* food::getPatch() { return patch; }
inline short food::domain() { return fDomain; }
inline void food::domain(short id) { fDomain = id; }


#endif



