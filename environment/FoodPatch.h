//---------------------------------------------------------------------------
//	File:		FoodPatch.h
//---------------------------------------------------------------------------

#pragma once

//System
#include <iostream>
#include <list>

//Local
#include "Energy.h"
#include "FoodType.h"
#include "gstage.h"
#include "Patch.h"

using namespace std;

// Forward declarations
class food;
class FoodPatch;

//===========================================================================
// FoodPatch
//===========================================================================
class FoodPatch : public Patch
{
 public:
	// forward decl
	class OnCondition;

	FoodPatch();
	~FoodPatch();

	float update();
	food *addFood( long step );
	void setInitCounts( int initFood, int minFood, int maxFood, int maxFoodGrown, float newFraction );
	void init( const FoodType *foodType, float x, float z, float sx, float sz, float rate, int initFood, int minFood, int maxFood, int maxFoodGrown, float patchFraction, int shape, int distribution, float nhsize, class OnCondition *onCondition, bool inRemoveFood, gstage* fStage, Domain* dm, int domainNumber );
	void updateOn( long step );
	bool on( long step );
	bool turnedOff( long step );
	bool initFoodGrown();
	void initFoodGrown( bool setInitFoodGrown );

	//===========================================================================
	// OnCondition
	//===========================================================================
	class OnCondition
	{
	public:
		virtual ~OnCondition() {}
		virtual void updateOn( long step ) = 0;
		virtual bool on( long step ) = 0;
		virtual bool turnedOff( long step ) = 0;
	};

	//===========================================================================
	// TimeOnCondition
	//
	// This condition controls whether a patch is on via period, phase,
	// and onFraction.
	//===========================================================================
	class TimeOnCondition : public OnCondition
	{
	public:
		TimeOnCondition( int period,
						 float onFraction,
						 float phase );
		virtual ~TimeOnCondition();
		virtual void updateOn( long step );
		virtual bool on( long step );
		virtual bool turnedOff( long step );

	private:
		int period;
		float inversePeriod;
		float onFraction;
		float phase;
	};

	//===========================================================================
	// MaxPopGroupOnCondition
	//===========================================================================
	class MaxPopGroupOnCondition : public OnCondition
	{
	public:
		typedef std::list<MaxPopGroupOnCondition *> MemberList;
		struct Group
		{
			MemberList members;

			static void validate( Group *group );
			void findNext( long step, MaxPopGroupOnCondition *exclude = NULL );
		};

		MaxPopGroupOnCondition( FoodPatch *patch,
								Group *group,
								int maxPopulation,
								int timeout,
								int delay );
		virtual ~MaxPopGroupOnCondition();
		virtual void updateOn( long step );
		virtual bool on( long step );
		virtual bool turnedOff( long step );

	private:
		// This is the data we're configured with.
		struct Parameters
		{
			FoodPatch *patch;
			Group *group;
			int maxPopulation;
			int timeout;
			int delay;
		} parms;

		// This is the data that changes during a sim.
		struct State
		{
			int start;
			int end;
			int findNext;
		} state;
	};

	float growthRate;
    
	int foodCount;
	int initFoodCount;
	int minFoodCount;
	int maxFoodCount;
	int maxFoodGrownCount;

	float fraction;    
	float foodRate;

	bool removeFood;
	bool foodGrown;

 private:
	OnCondition *onCondition;
	const FoodType *foodType;
};

inline void FoodPatch::updateOn( long step )
{
	return onCondition->updateOn( step );
}

inline bool FoodPatch::on( long step )
{
	return onCondition->on( step );
}

inline bool FoodPatch::turnedOff( long step )
{
	return onCondition->turnedOff( step );
}

inline bool FoodPatch::initFoodGrown( void )
{
	return( foodGrown );
}

inline void FoodPatch::initFoodGrown( bool setInitFoodGrown )
{
	foodGrown = setInitFoodGrown;
}
