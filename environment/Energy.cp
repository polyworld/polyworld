#include "Energy.h"

#include <assert.h>
#include <string.h>

#include <limits>

#include "globals.h"
#include "misc.h"
#include "proplib.h"

using namespace std;


#define EPSILON 0.00001

EnergyPolarity::EnergyPolarity()
{
	assert( globals::numEnergyTypes <= MAX_ENERGY_TYPES );

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = POSITIVE;
}

EnergyPolarity::EnergyPolarity( proplib::Property &prop )
{
	assert( prop.getType() == proplib::Node::Array );
	assert( (int)prop.elements().size() == globals::numEnergyTypes );

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = (Polarity)(int)prop.get( i );
}

bool EnergyPolarity::operator==( const EnergyPolarity &other ) const
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		if( values[i] != other.values[i] )
			return false;
	}

	return true;
}


EnergyMultiplier::EnergyMultiplier()
{
	assert( globals::numEnergyTypes <= MAX_ENERGY_TYPES );

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = 1;
}

EnergyMultiplier::EnergyMultiplier( float *values )
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
		this->values[i] = values[i];
}

EnergyMultiplier::EnergyMultiplier( proplib::Property &prop )
{
	assert( prop.getType() == proplib::Node::Array );
	assert( (int)prop.elements().size() == globals::numEnergyTypes );

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = (float)prop.get( i );
}

float EnergyMultiplier::operator[]( int i ) const
{
	return values[i];
}

bool operator==( const EnergyMultiplier &a, const EnergyMultiplier &b )
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
		if( fabs( a[i] - b[i] ) > EPSILON )
			return false;

	return true;
}

bool operator!=( const EnergyMultiplier &a, const EnergyMultiplier &b )
{
	return !(a == b);
}


Energy::Energy()
{
	init( 0.0f );
}

Energy::Energy( float val )
{
	init( val );
}

Energy::Energy( proplib::Property &prop )
{
	assert( prop.getType() == proplib::Node::Array );
	assert( (int)prop.elements().size() == globals::numEnergyTypes );

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = (float)prop.get( i );
}


void Energy::init( float val )
{
	assert( globals::numEnergyTypes <= MAX_ENERGY_TYPES );

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = val;
}

bool Energy::isDepleted() const
{
	return isDepleted( EPSILON );
}

bool Energy::isDepleted( const Energy &threshold ) const
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		if( values[i] <= threshold.values[i] )
			return true;
	}

	return false;
}

bool Energy::isZero() const
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		if( (values[i] < -EPSILON) || (values[i] > EPSILON) )
			return false;
	}

	return true;
}

float Energy::sum() const
{
	float result = 0;

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		result += values[i];

	return result;
}

float Energy::mean() const
{
	return sum() / globals::numEnergyTypes;
}

void Energy::zero()
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
		values[i] = 0.0;
}

void Energy::constrain( const Energy &minEnergy, const Energy &maxEnergy )
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		if( values[i] < minEnergy.values[i] )
		{
			values[i] = minEnergy.values[i];
		}
		else if( values[i] > maxEnergy.values[i] )
		{
			values[i] = maxEnergy.values[i];
		}
	}
}

void Energy::constrain( const Energy &minEnergy, const Energy &maxEnergy, Energy &result_overflow )
{
	result_overflow = 0;

	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		float diff;

		diff = values[i] - minEnergy.values[i];
		if( diff < 0 )
		{
			result_overflow.values[i] = diff;
			values[i] = minEnergy.values[i];
		}
		else
		{
			diff = values[i] - maxEnergy.values[i];
			if( diff > 0 )
			{
				result_overflow.values[i] = diff;
				values[i] = maxEnergy.values[i];
			}
		}
	}
}

Energy Energy::createDepletionThreshold( const Energy &threshold, const EnergyPolarity &polarity )
{
	Energy result = threshold;

	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		if( polarity.values[i] == EnergyPolarity::UNDEFINED )
			result.values[i] = numeric_limits<float>::quiet_NaN();
	}

	return result;
}

float Energy::operator[]( int index ) const
{
	return values[index];
}

Energy &Energy::operator+=( const Energy &other )
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		values[i] += other.values[i];
	}	

	return *this;
}

Energy &Energy::operator-=( const Energy &other )
{
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		values[i] -= other.values[i];
	}	

	return *this;
}

Energy operator+( const Energy &a, const Energy &b )
{
	Energy result;
	
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		result.values[i] = a.values[i] + b.values[i];
	}

	return result;
}

Energy operator-( const Energy &a, const Energy &b )
{
	Energy result;
	
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		result.values[i] = a.values[i] - b.values[i];
	}

	return result;
}

Energy operator*( const Energy &a, float val )
{
	Energy result;
	
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		result.values[i] = a.values[i] * val;
	}

	return result;
}

Energy operator*( float val, const Energy &a )
{
	return a * val;
}

Energy operator*( const Energy &e, const EnergyPolarity &p )
{
	Energy result;

	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		result.values[i] = e.values[i] * p.values[i];
	}

	return result;	
}

Energy operator*( const Energy &e, const EnergyMultiplier &m )
{
	Energy result;

	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		if( (m[i] != 0) && ( sign(m[i]) == sign(e.values[i]) ) )
			result.values[i] = e.values[i] * fabs( m.values[i] );
		else
			result.values[i] = e.values[i];
	}

	return result;	
}

ostream &operator<<( ostream &out, const Energy &e )
{
	out << "Energy(";
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		out << e.values[i];
		if( i != globals::numEnergyTypes - 1 )
			out << ",";
	}
	out << ")";

	return out;
}

void Energy::test()
{
	globals::numEnergyTypes = 3;

	{
		Energy e;
		assert( e.values[0] == 0 );
		assert( e.values[1] == 0 );
		assert( e.values[2] == 0 );
	}

	{
		Energy e( 2.0f );
		assert( e.values[0] == 2.0f );
		assert( e.values[1] == 2.0f );
		assert( e.values[2] == 2.0f );
	}

	{
		Energy e( 3.0f );
		Energy f = e;
		assert( f.values[0] == 3.0f );
		assert( f.values[1] == 3.0f );
		assert( f.values[2] == 3.0f );
	}

	{
		Energy e( 3.0f );
		Energy f;
		f.values[0] = 2.0f;
		f.values[1] = 3.0f;
		f.values[2] = 4.0f;

		Energy g = e - f;
		assert( g.values[0] == 1.0f );
		assert( g.values[1] == 0.0f );
		assert( g.values[2] == -1.0f );
	}

	{
		Energy e( 3.0f );
		EnergyPolarity p;
		p.values[0] = EnergyPolarity::NEGATIVE;
		p.values[1] = EnergyPolarity::POSITIVE;
		p.values[2] = EnergyPolarity::NEGATIVE;
		EnergyPolarity q;
		q.values[0] = EnergyPolarity::NEGATIVE;
		q.values[1] = EnergyPolarity::POSITIVE;
		q.values[2] = EnergyPolarity::POSITIVE;

		Energy f = e * p * q;

		assert( f.values[0] == 3.0f );
		assert( f.values[1] == 3.0f );
		assert( f.values[2] == -3.0f );		
	}

	{
		Energy e( 3.0f );
		Energy f( 1.0f );

		e += f;

		assert( e.values[0] == 4.0f );
		assert( e.values[1] == 4.0f );
		assert( e.values[2] == 4.0f );		
	}

	{
		Energy e;

		e.values[0] = -5;
		e.values[1] = -2;
		e.values[2] = 2;

		e.constrain( -2, 1 );

		assert( e.values[0] == -2 );
		assert( e.values[1] == -2 );
		assert( e.values[2] == 1 );
	}

	{
		Energy e(2);
		EnergyMultiplier m;

		m.values[0] = 1;
		m.values[1] = 0.5;
		m.values[2] = -2;
		
		Energy f = e * m;

		assert( f.values[0] == 2 );
		assert( f.values[1] == 1 );
		assert( f.values[2] == 2 );
	}
}
