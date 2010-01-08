#include "Variant.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>


Variant::Variant()
{
	type = INVALID;
}

Variant::Variant( int ival )
{
	type = INT;
	this->ival = ival;
}

Variant::Variant( long ival )
{
	type = INT;
	this->ival = ival;
}

Variant::Variant( float fval )
{
	type = FLOAT;
	this->fval = fval;
}

Variant::Variant( double fval )
{
	type = FLOAT;
	this->fval = fval;
}

Variant::Variant( bool bval )
{
	type = BOOL;
	this->bval = bval;
}

Variant::Variant( const char *sval )
{
	type = STRING;
	this->sval = strdup( sval );
}

Variant::Variant( const Variant &variant )
{
	*this = variant;
}

Variant &Variant::operator = ( const Variant &variant )
{
	type = variant.type;
	switch(type)
	{
	case INVALID:
		// no-op
		break;
	case INT:
		ival = variant.ival;
		break;
	case FLOAT:
		fval = variant.fval;
		break;
	case BOOL:
		bval = variant.bval;
		break;
	case STRING:
		sval = strdup( variant.sval );
		break;
	default:
		assert(false);
	}

	return *this;
}

Variant::~Variant()
{
	if( type == STRING )
	{
		free( const_cast<char *>(sval) );
		sval = NULL;
	}
}

Variant::operator int () const
{
	assert( type == INT );

	return ival;
}

Variant::operator long () const
{
	return (int)*this;
}

Variant::operator float () const
{
	assert( type == FLOAT );

	return fval;
}

Variant::operator double () const
{
	return (float)*this;
}

Variant::operator bool () const
{
	assert( type == BOOL );

	return bval;
}

Variant::operator const char *() const
{
	assert( type == STRING );

	return sval;
}
