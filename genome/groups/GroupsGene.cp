#include "GroupsGene.h"

#include <assert.h>

#include "GroupsGenome.h"
#include "GroupsGenomeSchema.h"

using namespace genome;
using namespace std;

// ================================================================================
// ===
// === CLASS GroupsGeneType
// ===
// ================================================================================
const GeneType *GroupsGeneType::NEURGROUP = new GeneType();
const GeneType *GroupsGeneType::NEURGROUP_ATTR = new GeneType();
const GeneType *GroupsGeneType::SYNAPSE_ATTR = new GeneType();

#define CAST_TO(TYPE)											\
	TYPE##Gene *GroupsGeneType::to_##TYPE( Gene *gene_ )		\
	{															\
		assert( gene_ );										\
		TYPE##Gene *gene = dynamic_cast<TYPE##Gene *>( gene_ );	\
		assert( gene ); /* catch cast failure */				\
		return gene;											\
	}

CAST_TO(NeurGroup);
CAST_TO(NeurGroupAttr);
CAST_TO(SynapseAttr);

#undef CAST_TO


// ================================================================================
// ===
// === CLASS NeurGroupGene
// ===
// ================================================================================
NeurGroupGene::NeurGroupGene( NeurGroupType _group_type )
{
	first_group = -1; // this will be set by the schema
	group_type = _group_type;

	switch(group_type)
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
	case NGT_INTERNAL:
		// okay
		break;
	default:
		assert(false);
	}
}

bool NeurGroupGene::isMember( NeurGroupType _group_type )
{
	if( (_group_type == NGT_ANY) || (_group_type == this->group_type) )
	{
		return true;
	}

	switch( this->group_type )
	{
	case NGT_INPUT:
		return false;
	case NGT_OUTPUT:
	case NGT_INTERNAL:
		return (_group_type == NGT_ANY) || (_group_type == NGT_NONINPUT);
	default:
		assert(false);
	}
}

NeurGroupType NeurGroupGene::getGroupType()
{
	return group_type;
}


// ================================================================================
// ===
// === CLASS MutableNeurGroupGene
// ===
// ================================================================================
MutableNeurGroupGene::MutableNeurGroupGene( const char *_name,
											NeurGroupType _group_type,
											Gene *_gmin,
											Gene *_gmax )
: NeurGroupGene( _group_type )
, __InterpolatedGene( GroupsGeneType::NEURGROUP,
					  true /* mutable */,
					  _name,
					  _gmin,
					  _gmax,
					  ROUND_INT_NEAREST )
{
}

Scalar MutableNeurGroupGene::get( Genome *genome )
{
	return interpolate( genome->get_raw(offset) );
}

int MutableNeurGroupGene::getMaxGroupCount()
{
	switch( getGroupType() )
	{
	case NGT_INPUT:
		return 1;
	case NGT_INTERNAL: {
		return getMax();
	}
	default:
		assert(false);
	}
}

int MutableNeurGroupGene::getMaxNeuronCount()
{
	switch( getGroupType() )
	{
	case NGT_INPUT:
		return getMax();
	case NGT_INTERNAL: {
		int ngroups = getMaxGroupCount();
		int numineur = GroupsGeneType::to_NeurGroupAttr(schema->get("InhibitoryNeuronCount"))->getMax();
		int numeneur = GroupsGeneType::to_NeurGroupAttr(schema->get("ExcitatoryNeuronCount"))->getMax();

		return ngroups * (numineur + numeneur);
	}
	default:
		assert(false);
	}
}

std::string MutableNeurGroupGene::getTitle( int group )
{
	group -= first_group;

	assert( group >= 0 );
	assert( group < getMaxGroupCount() );

	switch( getGroupType() )
	{
	case NGT_INPUT:
		// we're assuming only 1 group when titling input
		assert( group == 0 );
		return name;
	case NGT_INTERNAL: {
		char buf[1024];
		sprintf( buf, "InternalNeurGroup %d", group );

		return buf;
	}
	default:
		assert(false);
	}
}


// ================================================================================
// ===
// === CLASS ImmutableNeurGroupGene
// ===
// ================================================================================
ImmutableNeurGroupGene::ImmutableNeurGroupGene( const char *_name,
												NeurGroupType _group_type )
: NeurGroupGene( _group_type )
, __ConstantGene( GroupsGeneType::NEURGROUP,
				  _name,
				  1 )
{
}

Scalar ImmutableNeurGroupGene::get( Genome *genome )
{
	return 1;
}

int ImmutableNeurGroupGene::getMaxGroupCount()
{
	return 1;
}

int ImmutableNeurGroupGene::getMaxNeuronCount()
{
	return 1;
}

std::string ImmutableNeurGroupGene::getTitle( int group )
{
	assert( group == first_group );

	return name;
}


// ================================================================================
// ===
// === CLASS NeurGroupAttrGene
// ===
// ================================================================================
NeurGroupAttrGene::NeurGroupAttrGene( const char *_name,
									  NeurGroupType _group_type,
									  Gene *_gmin,
									  Gene *_gmax )
: __InterpolatedGene( GroupsGeneType::NEURGROUP_ATTR,
					  true /* mutable */,
					  _name,
					  _gmin,
					  _gmax,
					  ROUND_INT_NEAREST )
, group_type( _group_type )
{
}

const Scalar &NeurGroupAttrGene::getMin()
{
	return __InterpolatedGene::getMin();
}

const Scalar &NeurGroupAttrGene::getMax()
{
	return __InterpolatedGene::getMax();
}

Scalar NeurGroupAttrGene::get( Genome *genome,
							   int group )
{
	int offset = getOffset( group );

	return interpolate( genome->get_raw(offset) );
}

void NeurGroupAttrGene::seed( Genome *genome,
							  NeurGroupGene *group,
							  unsigned char rawval )
{
	int igroup = dynamic_cast<GroupsGenomeSchema *>(schema)->getFirstGroup( group );
	int offset = getOffset( igroup );
	int ngroups = group->getMaxGroupCount();

	genome->set_raw( offset,
					 ngroups * sizeof(unsigned char),
					 rawval );
}

void NeurGroupAttrGene::printIndexes( FILE *file, GenomeLayout *layout )
{
	int n = dynamic_cast<GroupsGenomeSchema *>(schema)->getMaxGroupCount( group_type );

	for( int i = 0; i < n; i++ )
	{
		int index = offset + i;
		if( layout )
		{
			index = layout->getMutableDataOffset( index );
		}

		fprintf( file, "%d\t%s_%d\n", index, name.c_str(), i );
	}
}

void NeurGroupAttrGene::printTitles( FILE *file )
{
	int first_group = dynamic_cast<GroupsGenomeSchema *>(schema)->getFirstGroup( group_type );
	int ngroups = dynamic_cast<GroupsGenomeSchema *>(schema)->getMaxGroupCount( group_type );

	for( int i = 0; i < ngroups; i++ )
	{
		int group = first_group + i;
		NeurGroupGene *groupGene = dynamic_cast<GroupsGenomeSchema *>(schema)->getGroupGene( group );
		string groupTitle = groupGene->getTitle( group );

		fprintf( file,
				 "%s[%s] :: %s_%d\n",
				 name.c_str(), groupTitle.c_str(),
				 name.c_str(), i );
	}
}

int NeurGroupAttrGene::getMutableSizeImpl()
{
	return sizeof(unsigned char) * dynamic_cast<GroupsGenomeSchema *>(schema)->getMaxGroupCount( group_type );
}

int NeurGroupAttrGene::getOffset( int group )
{
	return this->offset + (group - dynamic_cast<GroupsGenomeSchema *>(schema)->getFirstGroup( group_type ));
}


// ================================================================================
// ===
// === CLASS SynapseAttrGene
// ===
// ================================================================================
SynapseAttrGene::SynapseAttrGene( const char *_name,
								  bool _negateInhibitory,
								  bool _lessThanZero,
								  Gene *_gmin,
								  Gene *_gmax )
: __InterpolatedGene( GroupsGeneType::SYNAPSE_ATTR,
					  true /* mutable */,
					  _name,
					  _gmin,
					  _gmax,
					  ROUND_INT_NEAREST )
, negateInhibitory( _negateInhibitory )
, lessThanZero( _lessThanZero )
{
}

Scalar SynapseAttrGene::get( Genome *genome,
							 GroupsSynapseType *synapseType,
							 int group_from,
							 int group_to )
{
	int offset = getOffset( synapseType,
							group_from,
							group_to );

	Scalar result = interpolate( genome->get_raw(offset) );

	if( negateInhibitory && synapseType->nt_from == INHIBITORY )
	{
		if( lessThanZero )
		{
			// value must be < 0
			return min(-1.e-10, -(double)result);
		}
		else
		{
			return -(float)result;
		}
	}
	else
	{
		return result;
	}
}

void SynapseAttrGene::seed( Genome *genome,
							GroupsSynapseType *synapseType,
							NeurGroupGene *from,
							NeurGroupGene *to,
							unsigned char rawval )
{
	assert( (from->getGroupType() == NGT_INPUT) || (from->getGroupType() == NGT_OUTPUT) );
	assert( to->getGroupType() == NGT_OUTPUT );

	int offset = getOffset( synapseType,
							dynamic_cast<GroupsGenomeSchema *>(schema)->getFirstGroup(from),
							dynamic_cast<GroupsGenomeSchema *>(schema)->getFirstGroup(to) );

	genome->set_raw( offset,
					 sizeof(unsigned char),
					 rawval );
}

void SynapseAttrGene::printIndexes( FILE *file, GenomeLayout *layout )
{
	int nin = dynamic_cast<GroupsGenomeSchema *>(schema)->getMaxGroupCount( NGT_INPUT );
	int nany = dynamic_cast<GroupsGenomeSchema *>(schema)->getMaxGroupCount( NGT_ANY );

	for( GroupsSynapseTypeList::const_iterator
			 it = dynamic_cast<GroupsGenomeSchema *>(schema)->getSynapseTypes().begin(),
			 end = dynamic_cast<GroupsGenomeSchema *>(schema)->getSynapseTypes().end();
		 it != end;
		 it++ )
	{
		GroupsSynapseType *st = *it;

		for( int to = nin; to < nany; to++ )
		{
			for( int from = 0; from < nany; from++ )
			{
				int index = getOffset( st, from, to );
				if( layout )
				{
					index = layout->getMutableDataOffset( index );
				}

				fprintf( file, "%d\t%s%s_%d->%d\n",
						 index,
						 st->name.c_str(), this->name.c_str(),
						 from, to );
			}
		}
	}	
}

void SynapseAttrGene::printTitles( FILE *file )
{
	// we don't currently investigate this data, not implemented yet.
}

int SynapseAttrGene::getMutableSizeImpl()
{
	int size = 0;

	for( GroupsSynapseTypeList::const_iterator
			 it = dynamic_cast<GroupsGenomeSchema *>(schema)->getSynapseTypes().begin(),
			 end = dynamic_cast<GroupsGenomeSchema *>(schema)->getSynapseTypes().end();
		 it != end;
		 it++ )
	{
		size += (*it)->getMutableSize();
	}

	return size;
}

int SynapseAttrGene::getOffset( GroupsSynapseType *synapseType,
								int from,
								int to )
{
	return this->offset + synapseType->getOffset( from, to );
}
