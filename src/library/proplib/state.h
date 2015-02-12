#pragma once

#include <vector>

#include "cppprops.h"
#include "FoodPatch.h"


namespace proplib
{
		class __StateObject
		{
		protected:
			static CppProperties::UpdateContext *getUpdateContext();
			static long getStep();
		};

		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- CLASS Periodic
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		class Periodic : __StateObject
		{
		public:
			Periodic( int period,
					  float onFraction,
					  float phase );
			virtual ~Periodic();
			bool update();

		private:
			int _period;
			float _inversePeriod;
			float _onFraction;
			float _phase;
		};

		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- CLASS FoodPatchTokenRing
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		class FoodPatchTokenRing : __StateObject
		{
		public:
			static void add( FoodPatch &patch, int maxPopulation = -1, int timeout = -1, int delay = -1 );
			static bool update( FoodPatch &patch );

		private:
			class Member
			{
			public:
				FoodPatch *patch;
				int start;
				int end;
			};
			static void updateActive();
			static void findActive( bool killAgents, Member *exclude );

			static int _maxPopulation;
			static int _timeout;
			static int _delay;

			static long _step;
			static long _delayEnd;
			static std::vector<Member *> _members;
			static Member *_active;
		};

}
