#pragma once

class RandomNumberGenerator
{
 public:
	// ---
	// --- ENUMS
	// ---
	enum Role
	{
		NERVOUS_SYSTEM = 0,
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
	friend class ModuleInit;

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
	double drand();
	double range( double lo,
				  double hi );

 private:
	Type type;
	void *state;
};
