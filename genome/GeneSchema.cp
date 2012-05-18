#include "GeneSchema.h"

#include <assert.h>
#include <stdlib.h>

#include "agent.h"
#include "Metabolism.h"
#include "misc.h"

using namespace genome;
using namespace std;


// ================================================================================
// ===
// === CLASS GeneSchema
// ===
// ================================================================================

//-------------------------------------------------------------------------------------------
// GeneSchema::GeneSchema
//-------------------------------------------------------------------------------------------
GeneSchema::GeneSchema()
{
	_state = STATE_CONSTRUCTING;
	_offset = -1;
}

//-------------------------------------------------------------------------------------------
// GeneSchema::~GeneSchema
//-------------------------------------------------------------------------------------------
GeneSchema::~GeneSchema()
{
	itfor( GeneVector, _genes, it )
	{
		delete (*it);
	}
}

//-------------------------------------------------------------------------------------------
// GeneSchema::add
//-------------------------------------------------------------------------------------------
Gene *GeneSchema::add( Gene *gene )
{
	assert( _state == STATE_CONSTRUCTING );
	assert( get(gene->name) == NULL );

	_name2gene[gene->name] = gene;
	_type2genes[gene->type].push_back( gene );
	_genes.push_back( gene );

	return gene;
}

//-------------------------------------------------------------------------------------------
// GeneSchema::get
//-------------------------------------------------------------------------------------------
Gene *GeneSchema::get( const string &name )
{
	return _name2gene[name];
}

//-------------------------------------------------------------------------------------------
// GeneSchema::get
//-------------------------------------------------------------------------------------------
Gene *GeneSchema::get( const char *name )
{
	return _name2gene[name];
}

//-------------------------------------------------------------------------------------------
// GeneSchema::get
//-------------------------------------------------------------------------------------------
Gene *GeneSchema::get( const GeneType *type )
{
	GeneVector &_genes = _type2genes[type];

	assert( _genes.size() == 1 );

	return _genes.front();
}

//-------------------------------------------------------------------------------------------
// GeneSchema::getAll
//-------------------------------------------------------------------------------------------
const GeneVector &GeneSchema::getAll( const GeneType *type )
{
	return _type2genes[type];
}

//-------------------------------------------------------------------------------------------
// GeneSchema::getAll
//-------------------------------------------------------------------------------------------
const GeneVector &GeneSchema::getAll()
{
	return _genes;
}

//-------------------------------------------------------------------------------------------
// GeneSchema::getGeneCount
//-------------------------------------------------------------------------------------------
int GeneSchema::getGeneCount( const GeneType *type )
{
	return _type2genes[type].size();
}

//-------------------------------------------------------------------------------------------
// GeneSchema::getMutableSize
//-------------------------------------------------------------------------------------------
int GeneSchema::getMutableSize()
{
	switch(_state)
	{
	case STATE_COMPLETE:
		return _cache.mutableSize;
	case STATE_CACHING: {
		assert( _offset >= 0 );
		_cache.mutableSize = 0;

		itfor( GeneVector, _genes, it )
		{
			Gene *g = *it;
			if( g->ismutable )
			{
				g->offset = _offset + _cache.mutableSize;

				if( g->type == GeneType::CONTAINER )
					GeneType::to_Container( g )->complete();

				_cache.mutableSize += g->getMutableSize();
			}
			else
			{
				g->offset = -1;
			}
		}

		return _cache.mutableSize;
	}
	default:
		assert(false);
	}
}

//-------------------------------------------------------------------------------------------
// GeneSchema::complete
//-------------------------------------------------------------------------------------------
void GeneSchema::complete( int offset )
{
	beginComplete( offset );
	endComplete();
}

//-------------------------------------------------------------------------------------------
// GeneSchema::beginComplete
//-------------------------------------------------------------------------------------------
void GeneSchema::beginComplete( int offset )
{
	assert(_state == STATE_CONSTRUCTING);

	_state = STATE_CACHING;
	_offset = offset;
}

//-------------------------------------------------------------------------------------------
// GeneSchema::endComplete
//-------------------------------------------------------------------------------------------
void GeneSchema::endComplete()
{
	assert( _state == STATE_CACHING );

	getMutableSize();

	_state = STATE_COMPLETE;
}

//-------------------------------------------------------------------------------------------
// GeneSchema::getIndexes
//-------------------------------------------------------------------------------------------
void GeneSchema::getIndexes( vector<string> &geneNames, vector<int> &result )
{
	itfor( vector<string>, geneNames, it )
	{
		Gene *gene = _name2gene[*it];
		if( gene == NULL )
		{
			result.push_back( -1 );
		}
		else
		{
			result.push_back( gene->offset );
		}
	}
}

//-------------------------------------------------------------------------------------------
// GeneSchema::printIndexes
//-------------------------------------------------------------------------------------------
void GeneSchema::printIndexes( FILE *f, GenomeLayout *layout, const string &prefix  )
{
	itfor( GeneVector, _genes, it )
		(*it)->printIndexes( f, prefix, layout );
}

//-------------------------------------------------------------------------------------------
// GeneSchema::printTitles
//-------------------------------------------------------------------------------------------
void GeneSchema::printTitles( FILE *f, const string &prefix )
{
	itfor( GeneVector, _genes, it )
		(*it)->printTitles( f, prefix );
}

//-------------------------------------------------------------------------------------------
// GeneSchema::printRanges
//-------------------------------------------------------------------------------------------
void GeneSchema::printRanges( FILE *f, const string &prefix )
{
	itfor( GeneVector, _genes, it )
		(*it)->printRanges( f, prefix );
}
