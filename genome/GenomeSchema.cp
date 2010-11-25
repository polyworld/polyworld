#include "GenomeSchema.h"

#include <assert.h>
#include <stdlib.h>

#include "misc.h"

using namespace genome;
using namespace std;


// ================================================================================
// ===
// === CLASS GenomeSchema
// ===
// ================================================================================
GenomeSchema::GenomeSchema()
{
#define SYNAPSE_TYPE(NAME) \
	{\
		SynapseType *st = new SynapseType( this, #NAME, synapseTypes.size() ); \
		synapseTypes.push_back( st );\
		synapseTypeMap[#NAME] = st;\
	}

	SYNAPSE_TYPE( EE );
	SYNAPSE_TYPE( EI );
	SYNAPSE_TYPE( II );
	SYNAPSE_TYPE( IE );

#undef SYNAPSE_TYPE

	state = STATE_CONSTRUCTING;
}

GenomeSchema::~GenomeSchema()
{
	for( GeneList::iterator
			 it = genes.begin(),
			 end = genes.end();
		 it != end;
		 it++ )
	{
		delete (*it);
	}

	for( SynapseTypeList::iterator
			 it = synapseTypes.begin(),
			 end = synapseTypes.end();
		 it != end;
		 it++ )
	{
		delete (*it);
	}
}

Gene *GenomeSchema::add( Gene *gene )
{
	assert( state == STATE_CONSTRUCTING );
	assert( gene->schema == NULL );
	assert( get(gene->name) == NULL );

	gene->schema = this;

	name2gene[gene->name] = gene;
	type2genes[gene->type].push_back( gene );
	genes.push_back( gene );

	if( gene->type == Gene::NEURGROUP )
	{
		neurgroups.push_back( gene );
	}

	return gene;
}

Gene *GenomeSchema::get( const string &name )
{
	return name2gene[name];
}

Gene *GenomeSchema::get( const char *name )
{
	return name2gene[name];
}

Gene *GenomeSchema::get( Gene::Type type )
{
	GeneVector &genes = type2genes[type];

	assert( genes.size() == 1 );

	return genes.front();
}

const GeneVector &GenomeSchema::getAll( Gene::Type type )
{
	return type2genes[type];
}

int GenomeSchema::getGeneCount( Gene::Type type )
{
	return type2genes[type].size();
}

int GenomeSchema::getPhysicalCount()
{
	switch(state)
	{
	case STATE_COMPLETE:
		return cache.physicalCount;
	case STATE_CACHING: {
		int n = 0;

		for( GeneList::iterator
				 it = genes.begin(),
				 end = genes.end();
			 it != end;
			 it++ )
		{
			Gene *g = *it;

			if( !g->ismutable )
			{
				continue;
			}

			switch( g->type )
			{
			case Gene::SCALAR:
				n++;
				break;
			default:
				return cache.physicalCount = n;
			}
		}

		assert(false); // fell through!
		return -1;
	}
	default:
		assert(false);
	}
}

int GenomeSchema::getMaxGroupCount( NeurGroupType group )
{
	switch(state)
	{
	case STATE_COMPLETE:
		return cache.groupCount[group];
	case STATE_CACHING: {
		int n = 0;

		for( GeneList::iterator
				 it = neurgroups.begin(),
				 end = neurgroups.end();
			 it != end;
			 it++ )
		{
			NeurGroupGene *gene = (*it)->to_NeurGroup();

			if( gene->isMember(group) )
			{
				n += gene->getMaxGroupCount();
			}
		}

		return cache.groupCount[group] = n;
	}
	default:
		assert(false);
	}
}

int GenomeSchema::getFirstGroup( Gene *_gene )
{
	NeurGroupGene *gene = _gene->to_NeurGroup();

	switch(state)
	{
	case STATE_COMPLETE:
		return gene->first_group;
	case STATE_CACHING: {
		int group = 0;

		for( GeneList::iterator
				 it = neurgroups.begin(),
				 end = neurgroups.end();
			 it != end;
			 it++ )
		{
			NeurGroupGene *other = (*it)->to_NeurGroup();
			if( other == gene )
			{
				break;
			}
			group += other->getMaxGroupCount();
		}

		return gene->first_group = group;
	}
	default:
		assert(false);
	}
}

int GenomeSchema::getFirstGroup( NeurGroupType group )
{
	switch(state)
	{
	case STATE_COMPLETE:
		return cache.groupStart[group];
	case STATE_CACHING: {
		int n;

		switch( group )
		{
		case NGT_ANY:
		case NGT_INPUT:
			n = 0;
			break;
		case NGT_OUTPUT:
		case NGT_NONINPUT:
			n = getMaxGroupCount( NGT_INPUT );
			break;
		case NGT_INTERNAL:
			n = getMaxGroupCount( NGT_INPUT ) + getMaxGroupCount( NGT_OUTPUT );
			break;
		default:
			assert( false );
		}

		return cache.groupStart[group] = n;
	}
	default:
		assert(false);
	}
}

NeurGroupGene *GenomeSchema::getGroupGene( int group )
{
	Gene *gene;

	switch( getNeurGroupType(group) )
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
		gene = type2genes[Gene::NEURGROUP][group];
		break;
	case NGT_INTERNAL:
		gene = type2genes[Gene::NEURGROUP].back();
		break;
	default:
		assert(false);
	}

	return gene->to_NeurGroup();
}

int GenomeSchema::getMaxNeuronCount( NeurGroupType group )
{
	switch(state)
	{
	case STATE_COMPLETE:
		return cache.neuronCount[group];
	case STATE_CACHING: {
		int n = 0;

		for( GeneList::iterator
				 it = neurgroups.begin(),
				 end = neurgroups.end();
			 it != end;
			 it++ )
		{
			NeurGroupGene *gene = (*it)->to_NeurGroup();

			if( gene->isMember(group) )
			{
				n += gene->getMaxNeuronCount();
			} 
		}

		return cache.neuronCount[group] = n;
	}
	default:
		assert(false);
	}
}

NeurGroupType GenomeSchema::getNeurGroupType( int group )
{
	if( group < getFirstGroup(NGT_OUTPUT) )
	{
		return NGT_INPUT;
	}
	else if( group < getFirstGroup(NGT_INTERNAL) )
	{
		return NGT_OUTPUT;
	}
	else
	{
		return NGT_INTERNAL;
	}
}

int GenomeSchema::getSynapseTypeCount()
{
	return synapseTypes.size();
}

int GenomeSchema::getMaxSynapseCount()
{
	switch(state)
	{
	case STATE_COMPLETE:
		return cache.synapseCount;
	case STATE_CACHING: {
		int input = getMaxNeuronCount( NGT_INPUT );
		int output = getMaxNeuronCount( NGT_OUTPUT );
		int internal = getMaxNeuronCount( NGT_INTERNAL );

		return cache.synapseCount =
			internal * internal
			+ 2 * output * output
			+ 3 * internal * output
			+ 2 * internal * input
			+ 2 * input * output
			- 2 * output
			- internal;
	}
	default:
		assert(false);
	}
}

const SynapseTypeList &GenomeSchema::getSynapseTypes()
{
	return synapseTypes;
}

SynapseType *GenomeSchema::getSynapseType( const char *name )
{
	return synapseTypeMap[name];
}

int GenomeSchema::getMutableSize()
{
	switch(state)
	{
	case STATE_COMPLETE:
		return cache.mutableSize;
	case STATE_CACHING: {
		cache.mutableSize = 0;

		for( GeneList::iterator
				 it = genes.begin(),
				 end = genes.end();
			 it != end;
			 it++ )
		{
			Gene *g = *it;
			if( g->ismutable )
			{
				g->offset = cache.mutableSize;
				cache.mutableSize += g->getMutableSize();
			}
			else
			{
				g->offset = -1;
			}
		}

		return cache.mutableSize;
	}
	default:
		assert(false);
	}
}

void GenomeSchema::complete()
{
	assert(state == STATE_CONSTRUCTING);

	state = STATE_CACHING;

#define NG_CACHE(NAME)								\
	getMaxGroupCount(NGT_##NAME);				\
	getFirstGroup(NGT_##NAME);					\
	getMaxNeuronCount(NGT_##NAME);

	NG_CACHE( ANY );
	NG_CACHE( INPUT );
	NG_CACHE( OUTPUT );
	NG_CACHE( INTERNAL );
	NG_CACHE( NONINPUT );

#undef NG_CACHE

	getMaxSynapseCount();
	getPhysicalCount();
	
	for( GeneList::iterator
			 it = neurgroups.begin(),
			 end = neurgroups.end();
		 it != end;
		 it++ )
	{
		Gene *gene = *it;

		getFirstGroup(gene);
	}

	for( SynapseTypeList::iterator
			 it = synapseTypes.begin(),
			 end = synapseTypes.end();
		 it != end;
		 it++ )
	{
		(*it)->complete( cache.groupCount );
	}

	getMutableSize();

	state = STATE_COMPLETE;
}

void GenomeSchema::printIndexes( FILE *f, GenomeLayout *layout )
{
	for( GeneList::iterator
			 it = genes.begin(),
			 end = genes.end();
		 it != end;
		 it++ )
	{
		(*it)->printIndexes( f, layout );
	}
}

void GenomeSchema::printTitles( FILE *f )
{
	itfor( GeneList, genes, it )
	{
		(*it)->printTitles( f );
	}
}
