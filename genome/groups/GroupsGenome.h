#pragma once

#include "Genome.h"
#include "GroupsGenomeSchema.h"

namespace genome
{

	class GroupsGenome : public Genome
	{
	public:
		GroupsSynapseType *EE;
		GroupsSynapseType *EI;
		GroupsSynapseType *IE;
		GroupsSynapseType *II;

		Gene *CONNECTION_DENSITY;
		Gene *TOPOLOGICAL_DISTORTION;
		Gene *LEARNING_RATE;
		Gene *INHIBITORY_COUNT;
		Gene *EXCITATORY_COUNT;
		Gene *BIAS;
		Gene *INTERNAL;

		Gene *TAU;

		Gene *SPIKING_A;
		Gene *SPIKING_B;
		Gene *SPIKING_C;
		Gene *SPIKING_D;

		GroupsGenome( GroupsGenomeSchema *schema,
					  GenomeLayout *layout );
		virtual ~GroupsGenome();

		Brain *createBrain( NervousSystem *cns );

		GroupsGenomeSchema *getSchema();

		int getGroupCount( NeurGroupType type );
		int getNeuronCount( NeuronType type,
							int group );
		int getNeuronCount( int group );

		const GroupsSynapseTypeList &getSynapseTypes();
		int getSynapseCount( GroupsSynapseType *synapseType,
							 int from,
							 int to );
		int getSynapseCount( int from,
							 int to );

		using Genome::get;

		Scalar get( Gene *gene,
					int group );

		Scalar get( Gene *gene,
					GroupsSynapseType *synapseType,
					int from,
					int to );

		using Genome::seed;

		void seed( Gene *attr,
				   Gene *group,
				   float rawval_ratio );

		void seed( Gene *gene,
				   GroupsSynapseType *synapseType,
				   Gene *from,
				   Gene *to,
				   float rawval_ratio );

	protected:
		virtual void getCrossoverPoints( long *crossoverPoints, long numCrossPoints );

	private:
		GroupsGenomeSchema *_schema;
	};
}

