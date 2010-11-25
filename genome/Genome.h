#pragma once

#include <iostream>

#include "GenomeSchema.h"

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
		~Genome();

		SynapseType *EE;
		SynapseType *EI;
		SynapseType *IE;
		SynapseType *II;

		Gene *CONNECTION_DENSITY;
		Gene *TOPOLOGICAL_DISTORTION;
		Gene *LEARNING_RATE;
		Gene *MISC_BIAS;
		Gene *MISC_INVIS_SLOPE;
		Gene *INHIBITORY_COUNT;
		Gene *EXCITATORY_COUNT;
		Gene *BIAS;
		Gene *BIAS_LEARNING_RATE;
		Gene *INTERNAL;

		Gene *gene( const char *name );

		Scalar get( const char *name );
		Scalar get( Gene *gene );
		Scalar get( Gene *gene,
					int group );
		Scalar get( Gene *gene,
					SynapseType *synapseType,
					int from,
					int to );

		unsigned int get_raw_uint( long byte );

		int getGroupCount( NeurGroupType type );
		int getNeuronCount( NeuronType type,
							int group );
		int getNeuronCount( int group );


		const SynapseTypeList &getSynapseTypes();

		int getSynapseCount( SynapseType *synapseType,
							 int from,
							 int to );
		int getSynapseCount( int from,
							 int to );

		void seed( Gene *gene,
				   float rawval_ratio );

		void seed( Gene *attr,
				   Gene *group,
				   float rawval_ratio );

		void seed( Gene *gene,
				   SynapseType *synapseType,
				   Gene *from,
				   Gene *to,
				   float rawval_ratio );

		void randomize( float bitonprob );
		void randomize();

		void mutate();
		void crossover( Genome *g1,
						Genome *g2,
						bool mutate );
		void copyFrom( Genome *g );
		float separation( Genome *g );
		float mateProbability( Genome *g );

		void dump( std::ostream &out );
		void load( std::istream &in );

		void print();
		void print( long lobit, long hibit );

		GenomeSchema *schema;

	protected:
		friend class Gene;
		friend class MutableScalarGene;
		friend class MutableNeurGroupGene;
		friend class NeurGroupAttrGene;
		friend class SynapseAttrGene;

		unsigned char get_raw( int offset );
		void set_raw( int offset,
					  int n,
					  unsigned char rawval );

	private:
		void alloc();

		GenomeLayout *layout;
		unsigned char *mutable_data;
		int nbytes;
		bool gray;
	};

} // namespace genome
