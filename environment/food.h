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

// Forward declarations
class food;

//===========================================================================
// TSortedFoodList
//===========================================================================

class fxsortedlist : public gdlist<food*>
{
public:
    fxsortedlist() { }
    ~fxsortedlist() { }
    void add(food* a);
    void sort();
};

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
	static float gSize2Energy; // (converts food/critter size to available energy)
	static fxsortedlist gXSortedFood;
		

    food();
    food(float e);
    food(float e, float x, float z);
    ~food();
    
	void dump(ostream& out);
	void load(istream& in);
    
	float eat(float e);
    
	float energy();
    short domain();
   	void domain(short id);

protected:
    void initfood();
    void initfood(float e);
    void initfood(float e, float x, float z);
	void initpos();
	void initlen();
	void initrest();
   	virtual void setradius();

    float fEnergy;
    short fDomain;
};

//===========================================================================
// inlines
//===========================================================================
inline float food::energy() { return fEnergy; }
inline short food::domain() { return fDomain; }
inline void food::domain(short id) { fDomain = id; }

#endif



