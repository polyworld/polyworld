#include "GenomeLayout.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "Gene.h"
#include "GenomeSchema.h"
#include "GroupsGene.h"
#include "GroupsGenomeSchema.h"
#include "misc.h"

using namespace std;

using namespace genome;

#if 1
#define DBPRINT(stmt)
#else
#define DBPRINT(stmt) stmt
#endif

GenomeLayout *GenomeLayout::create( GenomeSchema *schema,
									LayoutType type )
{
	GenomeLayout *layout = new GenomeLayout( schema );
	
	switch( type )
	{
	case None:
		createNone( layout );
		break;
	case NeurGroup:
		createNeurGroup( layout, dynamic_cast<GroupsGenomeSchema *>(schema) );
		break;
	default:
		assert( false );
	}

	layout->validate();

	return layout;
}

void GenomeLayout::createNone( GenomeLayout *layout )
{
	for( int i = 0; i < layout->numOffsets; i++ )
	{
		layout->geneOffset2mutableDataOffset[i] = i;
	}
}

void GenomeLayout::createNeurGroup( GenomeLayout *layout,
									GroupsGenomeSchema *schema )
{
	int __index = 0;
#define ADD(GENE_OFFSET,N)												\
	{																	\
		assert( N == 1 );												\
		for( int __i = 0; __i < N; __i++ )								\
		{																\
			layout->geneOffset2mutableDataOffset[ (GENE_OFFSET) + __i ] = (__index) + __i; \
		}																\
																		\
		__index += N;													\
	}
	
	// ---
	// --- SCALARS
	// --- 
	citfor( GeneVector, schema->getAll(GeneType::SCALAR), it )
	{
		Gene *gene = *it;
		if( gene->ismutable )
		{
			DBPRINT( cout << gene->name << " " << gene->offset << " -> " << __index << endl );
			ADD( gene->offset, gene->getMutableSize() );
		}
	}

	// ---
	// --- NEURGROUP
	// --- 
	citfor( GeneVector, schema->getAll(GroupsGeneType::NEURGROUP), it )
	{
		Gene *gene = *it;

		if( gene->ismutable )
		{
			DBPRINT( cout << gene->name << " " << gene->offset << " -> " << __index << endl );
			ADD( gene->offset, gene->getMutableSize() );
		}
	}

	const int MaxGroups = schema->getMaxGroupCount(NGT_ANY);

	// ---
	// --- NEURGROUP_ATTR & SYNAPSE_ATTR
	// --- 
	for( int group = 0; group < MaxGroups; group++ )
	{
		//NeurGroupType groupType = schema->getNeurGroupType( group );
		NeurGroupGene *groupGene = schema->getGroupGene( group );

		DBPRINT( cout << "--- " << group << ", type=" << groupType << endl );

		// ---
		// --- NEURGROUP_ATTR
		// --- 
		citfor( GeneVector, schema->getAll(GroupsGeneType::NEURGROUP_ATTR), it )
		{
			NeurGroupAttrGene *attrGene = GroupsGeneType::to_NeurGroupAttr(*it);

			if( groupGene->isMember(attrGene->group_type) )
			{
				if( attrGene->ismutable )
				{
					DBPRINT( cout << attrGene->name << " " << attrGene->offset << " -> " << __index << endl );
					// warn! assuming 1 byte per attr
					ADD( attrGene->getOffset(group), 1 );
				}
			}
		}

		// ---
		// --- SYNAPSE_ATTR
		// --- 
		for( int group_to = 0; group_to < MaxGroups; group_to++ )
		{
			if( NGT_INPUT != schema->getNeurGroupType(group_to) )
			{
				citfor( GroupsSynapseTypeList, schema->getSynapseTypes(), it_syntype )
				{
					GroupsSynapseType *syntype = *it_syntype;

					citfor( GeneVector, schema->getAll(GroupsGeneType::SYNAPSE_ATTR), it )
					{
						SynapseAttrGene *attrGene = GroupsGeneType::to_SynapseAttr(*it);

						DBPRINT( cout << attrGene->name << endl );

						// warn! assuming 1 byte per attr
						ADD( attrGene->getOffset(syntype,
												 group,
												 group_to),
							 1 );
					}
				}
			}
		}
	}

	DBPRINT( cout << "final __index=" << __index << endl );
}

GenomeLayout::GenomeLayout( GenomeSchema *schema )
{
	numOffsets = schema->getMutableSize();
	geneOffset2mutableDataOffset = new int[ numOffsets ];

	for( int i = 0; i < numOffsets; i++ )
	{
		geneOffset2mutableDataOffset[i] = -1;
	}
}

GenomeLayout::~GenomeLayout()
{
	delete geneOffset2mutableDataOffset;
}

void GenomeLayout::validate()
{
	int present[ numOffsets ];
	memset( present, 0, sizeof(present) );

	for( int i = 0; i < numOffsets; i++ )
	{
		present[ getMutableDataOffset(i) ]++;
	}

	bool failed = false;
	for( int i = 0; i < numOffsets; i++ )
	{
		if( present[i] != 1 )
		{
			cout << "GenomeLayout::validate(): [" << i << "]=" << present[i] << endl;
			failed = true;
		}
	}

	if( failed )
	{
		exit( 1 );
	}
}
