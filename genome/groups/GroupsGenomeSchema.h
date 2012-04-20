#pragma once

#include "GenomeSchema.h"
#include "GroupsGene.h"
#include "NeurGroupType.h"
#include "GroupsSynapseType.h"

namespace genome
{
	class GroupsGenomeSchema : public GenomeSchema
	{
	public:
		GroupsGenomeSchema();
		virtual ~GroupsGenomeSchema();

		virtual void defineImmutable();
		virtual void defineMutable();
		virtual void seed( Genome *genome );

		virtual Gene *add( Gene *gene );

		int getMaxGroupCount( NeurGroupType group );
		int getFirstGroup( Gene *gene );
		int getFirstGroup( NeurGroupType group );
		NeurGroupGene *getGroupGene( int group );
		int getMaxNeuronCount( NeurGroupType group );
		NeurGroupType getNeurGroupType( int group );

		int getSynapseTypeCount();
		const GroupsSynapseTypeList &getSynapseTypes();
		GroupsSynapseType *getSynapseType( const char *name );
		int getMaxSynapseCount();

		virtual void complete();

	private:
		GroupsSynapseTypeMap synapseTypeMap;
		GroupsSynapseTypeList synapseTypes;
		GeneList neurgroups;

		struct Cache
		{
			int groupCount[__NGT_COUNT];
			int groupStart[__NGT_COUNT];
			int neuronCount[__NGT_COUNT];
			int synapseCount;
		} cache;
	};
}
