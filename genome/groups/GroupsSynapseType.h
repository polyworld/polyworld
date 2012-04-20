#pragma once

#include <list>
#include <map>
#include <string>

#include "NeuronType.h"

namespace genome
{

	// forward decls
	class GroupsGenomeSchema;

	// ================================================================================
	// ===
	// === CLASS GroupsSynapseType
	// ===
	// ================================================================================
	class GroupsSynapseType
	{
	public:
		std::string name;
		NeuronType nt_from;
		NeuronType nt_to;

		int getOffset( int from, int to );

	private:
		friend class GroupsGenomeSchema;
		friend class SynapseAttrGene;

		GroupsSynapseType( GroupsGenomeSchema *schema,
						   const char *name,
						   int index );

		void complete( int *groupCount );

		int getMutableSize();

	private:
		GroupsGenomeSchema *schema;
		int index;
		int *groupCount;
		int mutableSize;
	};	

	typedef std::list<GroupsSynapseType *> GroupsSynapseTypeList;
	typedef std::map<std::string, GroupsSynapseType *> GroupsSynapseTypeMap;

}
