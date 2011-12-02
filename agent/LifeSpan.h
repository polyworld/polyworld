#pragma once

class LifeSpan
{
	// ---
	// --- METAENUM Birth
	// ---
#define BIRTH_METAENUM(TYPE)					\
	BIRTH_TYPE(INVALID)							\
		BIRTH_TYPE(SIMINIT)						\
		BIRTH_TYPE(CREATE)						\
		BIRTH_TYPE(NATURAL)						\
		BIRTH_TYPE(LOCKSTEP)					\
		BIRTH_TYPE(VIRTUAL)

	// ---
	// --- METAENUM Death
	// ---
#define DEATH_METAENUM(TYPE)					\
	DEATH_TYPE(INVALID)							\
		DEATH_TYPE(SIMEND)						\
		DEATH_TYPE(SMITE)						\
		DEATH_TYPE(PATCH)						\
		DEATH_TYPE(NATURAL)						\
		DEATH_TYPE(FIGHT)						\
		DEATH_TYPE(EAT)							\
		DEATH_TYPE(LOCKSTEP)					\
		DEATH_TYPE(RANDOM)

 public:
	// ---
	// --- ENUM BirthReason
	// ---
#define BIRTH_TYPE(TYPE) BR_##TYPE,
	enum BirthReason
	{
		BIRTH_METAENUM()
		__BR_NTYPES
	};
#undef BIRTH_TYPE

	// ---
	// --- ENUM DeathReason
	// ---
#define DEATH_TYPE(TYPE) DR_##TYPE,
	enum DeathReason
	{
		DEATH_METAENUM()
		__DR_NTYPES
	};
#undef DEATH_TYPE

	static const char *BR_NAMES[__BR_NTYPES];
	static const char *DR_NAMES[__DR_NTYPES];

 public:
	LifeSpan();

	void set_birth( long step,
					BirthReason birthReason );
				
	void set_death( long step,
					DeathReason deathReason );

 public:
	struct
	{
		BirthReason reason;
		long step;
	} birth;

	struct
	{
		DeathReason reason;
		long step;
	} death;
};
