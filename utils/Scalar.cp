#include "Scalar.h"

#include <assert.h>
#include <stdio.h>

using namespace std;

Scalar::Scalar()
{
	type = INVALID;
}

Scalar::Scalar( int ival )
{
	type = INT;
	this->ival = ival;
}

Scalar::Scalar( long ival )
{
	type = INT;
	this->ival = ival;
}

Scalar::Scalar( float fval )
{
	type = FLOAT;
	this->fval = fval;
}

Scalar::Scalar( double fval )
{
	type = FLOAT;
	this->fval = fval;
}

Scalar::Scalar( bool bval )
{
	type = BOOL;
	this->bval = bval;
}

Scalar::Scalar( const Scalar &scalar )
{
	type = scalar.type;
	switch(type)
	{
	case INVALID:
		// no-op
		break;
	case INT:
		ival = scalar.ival;
		break;
	case FLOAT:
		fval = scalar.fval;
		break;
	case BOOL:
		bval = scalar.bval;
		break;
	default:
		assert(false);
	}
}

string Scalar::str() const
{
	char buf[128];

	switch(type)
	{
	case INT:
		sprintf(buf, "INT %d", ival);
		break;
	case FLOAT:
		sprintf(buf, "FLOAT %f", fval);
		break;
	case BOOL:
		sprintf(buf, "BOOL %s", bval ? "true" : "false");
		break;
	default:
		sprintf(buf, "INVALID SCALAR");
		break;
	}

	return buf;
}

Scalar::operator int () const
{
	assert( type == INT );

	return ival;
}

Scalar::operator long () const
{
	return (int)*this;
}

Scalar::operator float () const
{
	assert( type == FLOAT );

	return fval;
}

Scalar::operator double () const
{
	return (float)*this;
}

Scalar::operator bool () const
{
	assert( type == BOOL );

	return bval;
}

int operator + ( const Scalar &scalar, int i )
{
	assert( scalar.type == Scalar::INT );

	return scalar.ival + i;
}

int operator + ( int i, const Scalar &scalar )
{
	return scalar + i;
}

float operator - ( const Scalar &scalar, float f )
{
	assert( scalar.type == Scalar::FLOAT );

	return scalar.fval - f;
}

float operator - ( float f, const Scalar &scalar )
{
	return scalar - f;
}

float operator * ( const Scalar &scalar, float f )
{
	assert( scalar.type == Scalar::FLOAT );

	return scalar.fval * f;
}

float operator * ( float f, const Scalar &scalar )
{
	return scalar * f;
}

double operator * ( const Scalar &scalar, double f )
{
	assert( scalar.type == Scalar::FLOAT );

	return scalar.fval * f;
}

double operator * ( double f, const Scalar &scalar )
{
	return scalar * f;
}

bool operator <= ( long i, const Scalar &scalar )
{
	return i <= (long)scalar;
}

bool operator <= ( const Scalar &scalar, long i )
{
	return (long)scalar <= i;
}

bool operator == ( const Scalar &scalar, double i )
{
	return (double)scalar == i;
}

bool operator == ( double i, const Scalar &scalar )
{
	return i == (double)scalar;
}

ostream &operator << ( ostream &out, const Scalar &scalar )
{
	out << scalar.str();
	return out;
}

