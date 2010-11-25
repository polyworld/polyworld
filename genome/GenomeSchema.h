#pragma once

#include <stdio.h>

#include <string>

#include "Gene.h"
#include "NeurGroupType.h"
#include "SynapseType.h"

namespace genome
{
	// forward decl
	class GenomeLayout;

	// ================================================================================
	// ===
	// === CLASS GenomeSchema
	// ===
	// ================================================================================
	class GenomeSchema
	{
	public:
		GenomeSchema();
		~GenomeSchema();

		Gene *add( Gene *gene );

		Gene *get( const std::string &name );
		Gene *get( const char *name );
		Gene *get( Gene::Type type );
		const GeneVector &getAll( Gene::Type type );

		int getGeneCount( Gene::Type type );

		int getPhysicalCount();

		int getMaxGroupCount( NeurGroupType group );
		int getFirstGroup( Gene *gene );
		int getFirstGroup( NeurGroupType group );
		NeurGroupGene *getGroupGene( int group );
		int getMaxNeuronCount( NeurGroupType group );
		NeurGroupType getNeurGroupType( int group );

		int getSynapseTypeCount();
		int getMaxSynapseCount();

		const SynapseTypeList &getSynapseTypes();
		SynapseType *getSynapseType( const char *name );

		int getMutableSize();

		void complete();

		void printIndexes( FILE *f, GenomeLayout *layout = NULL );
		void printTitles( FILE *f );

	private:
		GeneMap name2gene;
		GeneTypeMap type2genes;
		GeneList neurgroups;
		GeneList genes;

		SynapseTypeMap synapseTypeMap;
		SynapseTypeList synapseTypes;

		struct Cache
		{
			int groupCount[__NGT_COUNT];
			int groupStart[__NGT_COUNT];
			int neuronCount[__NGT_COUNT];
			int physicalCount;
			int synapseCount;
			int mutableSize;
		} cache;

		enum State
		{
			STATE_CONSTRUCTING,
			STATE_CACHING,
			STATE_COMPLETE
		} state;
	};

} // namespace genome
