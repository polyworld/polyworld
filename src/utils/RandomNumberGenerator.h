#pragma once

namespace __RandomNumberGenerator
{
	class ModuleInit;
}

class RandomNumberGenerator
{
 public:
	// ---
	// --- ENUMS
	// ---
	enum Role
	{
		NERVOUS_SYSTEM = 0,
		TOPOLOGICAL_DISTORTION,
		INIT_WEIGHT,
		__NROLES
	};

	enum Type
	{
		GLOBAL,
		LOCAL
	};

	// ---
	// --- STATIC
	// ---
 public:
	static void set( Role role,
					 Type type );

	static RandomNumberGenerator *create( Role role );
	static void dispose( RandomNumberGenerator *rng );

 private:
	friend class __RandomNumberGenerator::ModuleInit;

	static void init();

 private:
	static Type types[__NROLES];

	// ---
	// --- INSTANCE
	// ---
 private:
	RandomNumberGenerator( Type type );
	~RandomNumberGenerator();

 public:
	void seed( long x );
	void seedIfLocal( long x );
	double drand();
	double range( double lo,
				  double hi );

 private:
	Type type;
	void *state;
};
