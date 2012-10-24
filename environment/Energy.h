#pragma once

#include <iostream>

#include "cppprops.h"

// We allocate arrays for the maximum potential number of energy types so that
// we don't have to malloc an array for every Energy instantiated. We want class
// Energy to be fairly cheap to instantiate because we create a lot of temp instances.
// MAX_ENERGY_TYPES can be increased to any arbitrary value, but the larger it is, the
// more bytes need to be copied around.
#define MAX_ENERGY_TYPES 4

// forward decl
class Energy;
namespace proplib
{
	class Property;
}

class EnergyPolarity
{
 public:
	enum Polarity
	{
		NEGATIVE = -1,
		POSITIVE = 1,
		UNDEFINED = 0
	};

	EnergyPolarity();
	EnergyPolarity( proplib::Property &prop );

	bool operator==( const EnergyPolarity &other ) const;

 private:
	friend class Energy;
	friend Energy operator*( const Energy &e, const EnergyPolarity &p );

	Polarity values[MAX_ENERGY_TYPES];
};

class EnergyMultiplier
{
	PROPLIB_CPP_PROPERTIES

 public:
	EnergyMultiplier();
	EnergyMultiplier( float *values );
	EnergyMultiplier( proplib::Property &prop );

	float operator[]( int i ) const;

 private:
	friend class Energy;
	friend Energy operator*( const Energy &e, const EnergyMultiplier &m );
	friend bool operator==( const EnergyMultiplier &a, const EnergyMultiplier &b );
	friend bool operator!=( const EnergyMultiplier &a, const EnergyMultiplier &b );

	float values[MAX_ENERGY_TYPES];
};

bool operator==( const EnergyMultiplier &a, const EnergyMultiplier &b );
bool operator!=( const EnergyMultiplier &a, const EnergyMultiplier &b );

class Energy
{
	PROPLIB_CPP_PROPERTIES

 public:
	Energy();
	Energy( float val );
	Energy( proplib::Property &prop );

	bool isDepleted() const;
	bool isDepleted( const Energy &threshold ) const;
	bool isZero() const;
	float sum() const;
	float mean() const;
	void zero();

	void constrain( const Energy &minEnergy, const Energy &maxEnergy );
	void constrain( const Energy &minEnergy, const Energy &maxEnergy, Energy &result_overflow );

	static Energy createDepletionThreshold( const Energy &threshold, const EnergyPolarity &polarity );

	float operator[]( int index ) const;

	Energy &operator+=( const Energy &other );
	Energy &operator-=( const Energy &other );

	friend Energy operator+( const Energy &a, const Energy &b );
	friend Energy operator-( const Energy &a, const Energy &b );

	friend Energy operator*( const Energy &e, float val );
	friend Energy operator*( float val, const Energy &e );

	friend Energy operator*( const Energy &e, const EnergyPolarity &p );
	friend Energy operator*( const Energy &e, const EnergyMultiplier &m );

	friend std::ostream &operator<<( std::ostream &out, const Energy &e );

 private:
	void init( float val );

	float values[MAX_ENERGY_TYPES];

 public:
	static void test();
};

Energy operator+( const Energy &a, const Energy &b );
Energy operator-( const Energy &a, const Energy &b );
Energy operator*( const Energy &e, float val );
Energy operator*( float val, const Energy &e );
Energy operator*( const Energy &e, const EnergyPolarity &p );
Energy operator*( const Energy &e, const EnergyMultiplier &m );

std::ostream &operator<<( std::ostream &out, const Energy &e );
