//---------------------------------------------------------------------------
//	File:		food.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#pragma once

// System
#include <iostream>
#include <list>

using namespace std;

// Local
#include "Energy.h"
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
	static long gMaxLifeSpan;

	typedef list<food *> FoodList;
	static FoodList gAllFood;

    food( const FoodType *foodType, long step );
    food( const FoodType *foodType, long step, const Energy &e );
    food( const FoodType *foodType, long step, const Energy &e, float x, float z);
    ~food();
    
	void dump(ostream& out);
	void load(istream& in);
    
	Energy eat(const Energy &e);
    
	bool isDepleted();

	const Energy &getEnergy();
	const EnergyPolarity &getEnergyPolarity();

	void setPatch(FoodPatch* fp);
	FoodPatch* getPatch();

	short domain();
	void domain(short id);

	long getAge( long step );

protected:
    void initfood( const FoodType *foodType, long step );
    void initfood( const FoodType *foodType, long step, const Energy &e );
    void initfood( const FoodType *foodType, long step, const Energy &e, float x, float z );
	void initlen();
	void initrest();
   	virtual void setradius();
	
    static unsigned long fFoodEver;

	Energy fEnergy;
    short fDomain;

	FoodPatch* patch; // pointer to this food's patch
	const FoodType* foodType;

	long fCreationStep;
	// This iterator addresses this object in the global list. This
	// allows us to remove this object from the list without having to
	// search for it.
	FoodList::iterator fAllFoodIterator;
};

//===========================================================================
// inlines
//===========================================================================
inline const Energy &food::getEnergy() { return fEnergy; }
inline const EnergyPolarity &food::getEnergyPolarity() { return foodType->energyPolarity; }
inline void food::setPatch(FoodPatch* fp) { patch=fp; }
inline FoodPatch* food::getPatch() { return patch; }
inline short food::domain() { return fDomain; }
inline void food::domain(short id) { fDomain = id; }
