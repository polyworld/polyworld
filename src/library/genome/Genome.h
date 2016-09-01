#pragma once

#include <assert.h>

#include <iostream>

#include "GenomeLayout.h"
#include "GenomeSchema.h"
#include "utils/graybin.h"
#include "utils/misc.h"

// forward decl
class AbstractFile;
class Brain;
class NervousSystem;

namespace genome
{
	// forward decl
	class GenomeLayout;

	// ================================================================================
	// ===
	// === CLASS Genome
	// ===
	// ================================================================================
	class Genome
	{
	public:
		Genome( GenomeSchema *schema,
				GenomeLayout *layout );
		virtual ~Genome();

		virtual Brain *createBrain( NervousSystem *cns ) = 0;

		Gene *MISC_BIAS;
		Gene *MISC_INVIS_SLOPE;

		Gene *gene( const char *name );

		Scalar get( const char *name );
		Scalar get( Gene *gene );

		unsigned int get_raw_uint( long byte );
		void updateSum( unsigned long *sum, unsigned long *sum2 );

		void seed( Gene *gene,
				   float rawval_ratio );
		void seedRandom( Gene *gene,
						 float rawval_ratio_min,
						 float rawval_ratio_max );
		void seedAll( float rawval_ratio );

		void randomizeBits( float bitonprob );
		void randomizeBits();
		void randomizeBytes( float mean, float stdev );
		void randomizeBytes();
		void randomize();

		void mutateBits();
		void mutateOneByte( long byte, float stdev );
		void mutateBytes();
		void mutate();
		virtual void crossover( Genome *g1,
								Genome *g2,
								bool mutate );
		void copyFrom( Genome *g );
		float separation( Genome *g );
		float mateProbability( Genome *g );

		void dump( AbstractFile *out );
		void load( AbstractFile *in );

		void print();
		void print( long lobit, long hibit );

	protected:
		friend class Gene;
		friend class MutableScalarGene;
		friend class MutableNeurGroupGene;
		friend class NeurGroupAttrGene;
		friend class SynapseAttrGene;

		virtual void getCrossoverPoints( long *crossoverPoints, long numCrossPoints );

		unsigned char get_raw( int offset );
		void set_raw( int offset,
					  int n,
					  unsigned char rawval );
		void set_raw_random( int offset,
							 int n,
							 int min,
							 int max );

		int nbytes;
		unsigned char *mutable_data;

	private:
		void alloc();

		GenomeSchema *schema;
		GenomeLayout *layout;
		bool gray;
	};


//===========================================================================
// inlines
//===========================================================================
inline unsigned char Genome::get_raw( int offset )
{
	assert( offset >= 0 && offset < nbytes );

	int layoutOffset = layout->getMutableDataOffset( offset );
	unsigned char val = mutable_data[layoutOffset];
	if( gray )
	{
		val = binofgray[val];
	}

	return val;
}

inline void Genome::set_raw( int offset,
							 int n,
							 unsigned char val )
{
	assert( (offset >= 0) && (offset + n <= nbytes) );

	if( gray )
	{
		val = grayofbin[val];
	}

	for( int i = 0; i < n; i++ )
	{
		int layoutOffset = layout->getMutableDataOffset( offset + i );

		mutable_data[layoutOffset] = val;
	}
}

inline void Genome::set_raw_random( int offset,
									int n,
									int min,
									int max )
{
	assert( (offset >= 0) && (offset + n <= nbytes) );

	for( int i = 0; i < n; i++ )
	{
		int layoutOffset = layout->getMutableDataOffset( offset + i );

		mutable_data[layoutOffset] = (unsigned char)rrand( min, max + 1 );
	}
}

} // namespace genome
