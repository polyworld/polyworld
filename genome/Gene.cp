#include "Gene.h"

#include <assert.h>

#include "Genome.h"
#include "GenomeLayout.h"
#include "GenomeSchema.h"
#include "misc.h"

using namespace genome;
using namespace std;

// ================================================================================
// ===
// === CLASS GeneType
// ===
// ================================================================================
const GeneType *GeneType::SCALAR = new GeneType();

#define CAST_TO(TYPE)											\
	TYPE##Gene *GeneType::to_##TYPE( Gene *gene_ )				\
	{															\
		assert( gene_ );										\
		TYPE##Gene *gene = dynamic_cast<TYPE##Gene *>( gene_ );	\
		assert( gene ); /* catch cast failure */				\
		return gene;											\
	}
	CAST_TO(NonVector);
	CAST_TO(ImmutableScalar);
	CAST_TO(MutableScalar);
	CAST_TO(ImmutableInterpolated);
	CAST_TO(__Interpolated);
#undef CAST_TO

// ================================================================================
// ===
// === CLASS Gene
// ===
// ================================================================================
void Gene::seed( Genome *genome,
				 unsigned char rawval )
{
	assert( ismutable && offset > -1 );

	genome->set_raw( offset,
					 getMutableSize(),
					 rawval );
}

int Gene::getMutableSize()
{
	return ismutable ? getMutableSizeImpl() : 0;
}

void Gene::printIndexes( FILE *file, GenomeLayout *layout )
{
	if( !ismutable )
	{
		return;
	}

	int index = offset;
	if( layout )
	{
		index = layout->getMutableDataOffset( index );
	}

	fprintf( file, "%d\t%s\n", index, name.c_str() );	
}

void Gene::printTitles( FILE *file )
{
	if( !ismutable )
	{
		return;
	}

	fprintf( file, "%s :: %s\n", name.c_str(), name.c_str() );
}

void Gene::printRanges( FILE *file )
{
	if( !ismutable )
	{
		return;
	}

	fprintf( file, "%s :: %s\n", name.c_str(), name.c_str() );
}

void Gene::init( const GeneType *_type,
				 bool _ismutable,
				 const char *_name )
{
	schema = NULL;
	type = _type;
	ismutable = _ismutable;
	name = _name;
	offset = -1;
}

int Gene::getMutableSizeImpl()
{
	assert(false);
	return -1;
}


// ================================================================================
// ===
// === CLASS __ConstantGene
// ===
// ================================================================================
__ConstantGene::__ConstantGene( const GeneType *_type,
								const char *_name,
								const Scalar &_value )
: value( _value )
{
	init( _type,
		  false /* immutable */,
		  _name );
}

const Scalar &__ConstantGene::get()
{
	return value;
}


// ================================================================================
// ===
// === CLASS __InterpolatedGene
// ===
// ================================================================================
__InterpolatedGene::__InterpolatedGene( const GeneType *_type,
										bool _ismutable,
										const char *_name,
										Gene *_gmin,
										Gene *_gmax,
										Rounding _rounding )
: smin( GeneType::to_ImmutableScalar(_gmin)->get( NULL ) )
, smax( GeneType::to_ImmutableScalar(_gmax)->get( NULL ) )
, rounding( _rounding )
, interpolationPower( 1.0 )
{
	init( _type,
		  _ismutable,
		  _name );

	assert( smin.type == smax.type );
}

const Scalar &__InterpolatedGene::getMin()
{
	return smin;
}

const Scalar &__InterpolatedGene::getMax()
{
	return smax;
}

void __InterpolatedGene::setInterpolationPower( double power )
{
	interpolationPower = power;
}

Scalar __InterpolatedGene::interpolate( unsigned char raw )
{
	static const float OneOver255 = 1. / 255.;

	// temporarily cast to double for backwards compatibility
	double ratio = float(raw) * OneOver255;
	if( interpolationPower != 1.0 )
		ratio = pow( ratio, interpolationPower );

	return interpolate( ratio );
}

Scalar __InterpolatedGene::interpolate( double ratio )
{
	switch(smin.type)
	{
	case Scalar::INT: {
		switch( rounding )
		{
		case ROUND_INT_FLOOR:
			{
				return (int)interp( ratio, int(smin), int(smax) );
			}
		case ROUND_INT_NEAREST:
			{
				return nint( interp(ratio, int(smin), int(smax)) );
			}
		case ROUND_INT_BIN:
			{
				return min( (int)interp( ratio, int(smin), int(smax) + 1 ),
							int(smax) );
			}
		default:
			assert( false );
		}
	}
	case Scalar::FLOAT: {
		return (float)interp( ratio, float(smin), float(smax) );
	}
	default:
		assert(false);
	}
}

void __InterpolatedGene::printRanges( FILE *file )
{
	const char *roundingNames[] = { "None", "IntFloor", "IntNearest", "IntBin" };
	const char *roundingName = 
		smin.type == Scalar::INT ? roundingNames[rounding] : roundingNames[ROUND_NONE];

	fprintf( file,
			 "%s %s %s %s\n",
			 roundingName,
			 smin.str().c_str(),
			 smax.str().c_str(),
			 name.c_str() );
}


// ================================================================================
// ===
// === CLASS NonVectorGene
// ===
// ================================================================================
int NonVectorGene::getMutableSizeImpl()
{
	return sizeof(unsigned char);
}


// ================================================================================
// ===
// === CLASS ImmutableScalarGene
// ===
// ================================================================================
ImmutableScalarGene::ImmutableScalarGene( const char *name,
										  const Scalar &value )
: __ConstantGene( GeneType::SCALAR,
				  name,
				  value )
{
}

Scalar ImmutableScalarGene::get( Genome *genome )
{
	return __ConstantGene::get();
}


// ================================================================================
// ===
// === CLASS MutableScalarGene
// ===
// ================================================================================
MutableScalarGene::MutableScalarGene( const char *name,
									  Gene *gmin,
									  Gene *gmax,
									  __InterpolatedGene::Rounding rounding )
: __InterpolatedGene( GeneType::SCALAR,
					  true /* mutable */,
					  name,
					  gmin,
					  gmax,
					  rounding )
{
}

Scalar MutableScalarGene::get( Genome *genome )
{
	return interpolate( genome->get_raw(offset) );	
}

const Scalar &MutableScalarGene::getMin()
{
	return __InterpolatedGene::getMin();
}

const Scalar &MutableScalarGene::getMax()
{
	return __InterpolatedGene::getMax();
}

void MutableScalarGene::printRanges( FILE * file )
{
	 __InterpolatedGene::printRanges( file );
}


// ================================================================================
// ===
// === CLASS ImmutableInterpolatedGene
// ===
// ================================================================================
ImmutableInterpolatedGene::ImmutableInterpolatedGene( const char *name,
													  Gene *gmin,
													  Gene *gmax,
													  __InterpolatedGene::Rounding rounding )
: __InterpolatedGene( GeneType::SCALAR,
					  false /* immutable */,
					  name,
					  gmin,
					  gmax,
					  rounding )
{
}

Scalar ImmutableInterpolatedGene::interpolate( double ratio )
{
	return __InterpolatedGene::interpolate( ratio );
}
