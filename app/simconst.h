#pragma once

namespace sim
{
	static const int MAXDOMAINS = 10; // if you change this, you MUST change the schema file.
	static const int MAXMETABOLISMS = 10;
	static const int MAXFITNESSITEMS = 5;

	enum ObjectType
	{
		OT_AGENT = 0,
		OT_FOOD,
		OT_BRICK,
		OT_BARRIER,
		OT_EDGE
	};

	enum FitnessScope
	{
		FS_OVERALL,
		FS_RECENT
	};

	enum FitnessWeightType
	{
		FWT__COMPLEXITY,
		FWT__HEURISTIC
	};

	enum FoodEnergyStatType
	{
		FEST__IN,
		FEST__OUT
	};

	enum FoodEnergyStatScope
	{
		FESS__STEP,
		FESS__TOTAL,
		FESS__AVERAGE
	};

	enum FitnessStatType
	{
		FST__MAX_FITNESS,
		FST__CURRENT_MAX_FITNESS,
		FST__AVERAGE_FITNESS
	};

	enum AgentBirthType
	{
		ABT__CREATED,
		ABT__BORN,
		ABT__BORN_VIRTUAL
	};

// Used in logic related to Mating as well as ContactEntry
#define MATE__NIL						0
#define MATE__DESIRED					(1 << 0)
#define MATE__PREVENTED__PARTNER		(1 << 1)
#define MATE__PREVENTED__CARRY			(1 << 2)
#define MATE__PREVENTED__MATE_WAIT		(1 << 3)
#define MATE__PREVENTED__ENERGY			(1 << 4)
#define MATE__PREVENTED__EAT_MATE_SPAN	(1 << 5)
#define MATE__PREVENTED__EAT_MATE_MIN_DISTANCE	(1 << 6)
#define MATE__PREVENTED__OF1			(1 << 7)
#define MATE__PREVENTED__MAX_DOMAIN		(1 << 8)
#define MATE__PREVENTED__MAX_WORLD		(1 << 9)
#define MATE__PREVENTED__MAX_METABOLISM	(1 << 10)
#define MATE__PREVENTED__MISC			(1 << 11)
#define MATE__PREVENTED__MAX_VELOCITY	(1 << 12)


// Used in logic related to Fighting as well as ContactEntry
#define FIGHT__NIL						0
#define FIGHT__DESIRED					(1 << 0)
#define FIGHT__PREVENTED__CARRY			(1 << 1)
#define FIGHT__PREVENTED__SHIELD		(1 << 2)
#define FIGHT__PREVENTED__POWER			(1 << 3)


// Used in logic related to Giving as well as ContactEntry
#define GIVE__NIL						0
#define GIVE__DESIRED					(1 << 0)
#define GIVE__PREVENTED__CARRY			(1 << 1)
#define GIVE__PREVENTED__ENERGY			(1 << 2)

}
