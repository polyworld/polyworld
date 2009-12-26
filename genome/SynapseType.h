#pragma once

#include <list>
#include <map>
#include <string>

#include "NeuronType.h"

namespace genome
{

	// forward decls
	class GenomeSchema;

	// ================================================================================
	// ===
	// === CLASS SynapseType
	// ===
	// ================================================================================
	class SynapseType
	{
	public:
		std::string name;
		NeuronType nt_from;
		NeuronType nt_to;

		int getOffset( int from, int to );

	private:
		friend class GenomeSchema;
		friend class SynapseAttrGene;

		SynapseType( GenomeSchema *schema,
					 const char *name,
					 int index );

		void complete( int *groupCount );

		int getMutableSize();

	private:
		GenomeSchema *schema;
		int index;
		int *groupCount;
		int mutableSize;
	};	

	typedef std::list<SynapseType *> SynapseTypeList;
	typedef std::map<std::string, SynapseType *> SynapseTypeMap;

}
