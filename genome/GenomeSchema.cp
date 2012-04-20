#include "GenomeSchema.h"

#include <assert.h>
#include <stdlib.h>

#include "agent.h"
#include "Metabolism.h"
#include "misc.h"

using namespace genome;
using namespace std;


// ================================================================================
// ===
// === CLASS GenomeSchema
// ===
// ================================================================================

GenomeSchema::Configuration GenomeSchema::config;

//-------------------------------------------------------------------------------------------
// GenomeSchema::processWorldfile
//-------------------------------------------------------------------------------------------
void GenomeSchema::processWorldfile( proplib::Document &doc )
{
	{
		string layout = doc.get( "GenomeLayout" );
		if( layout == "N" )
			GenomeSchema::config.layoutType = genome::GenomeLayout::NEURGROUP;
		else if( layout == "L" )
			GenomeSchema::config.layoutType = genome::GenomeLayout::LEGACY;
		else
			assert( false );
	}
    GenomeSchema::config.minMutationRate = doc.get( "MinMutationRate" );
    GenomeSchema::config.maxMutationRate = doc.get( "MaxMutationRate" );
    GenomeSchema::config.minNumCpts = doc.get( "MinCrossoverPoints" );
    GenomeSchema::config.maxNumCpts = doc.get( "MaxCrossoverPoints" );
    GenomeSchema::config.miscBias = doc.get( "MiscegenationFunctionBias" );
    GenomeSchema::config.miscInvisSlope = doc.get( "MiscegenationFunctionInverseSlope" );
	{
		proplib::Property &propPowers = doc.get( "GeneInterpolationPower" );
		itfor( proplib::PropertyMap, propPowers.elements(), it )
		{
			GenomeSchema::config.geneInterpolationPower[ it->second->get("Name") ] = it->second->get( "Power" );
		}
	}
    GenomeSchema::config.minBitProb = doc.get( "MinInitialBitProb" );
    GenomeSchema::config.maxBitProb = doc.get( "MaxInitialBitProb" );
	GenomeSchema::config.seedMutationRate = doc.get( "SeedMutationRate" );
	GenomeSchema::config.seedFightBias = doc.get( "SeedFightBias" );
	GenomeSchema::config.seedFightExcitation = doc.get( "SeedFightExcitation" );
	GenomeSchema::config.seedGiveBias = doc.get( "SeedGiveBias" );
	GenomeSchema::config.seedPickupBias = doc.get( "SeedPickupBias" );
	GenomeSchema::config.seedDropBias = doc.get( "SeedDropBias" );
	GenomeSchema::config.seedPickupExcitation = doc.get( "SeedPickupExcitation" );
	GenomeSchema::config.seedDropExcitation = doc.get( "SeedDropExcitation" );
    GenomeSchema::config.grayCoding = doc.get( "GrayCoding" );
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::GenomeSchema
//-------------------------------------------------------------------------------------------
GenomeSchema::GenomeSchema()
{
	state = STATE_CONSTRUCTING;
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::~GenomeSchema
//-------------------------------------------------------------------------------------------
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
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::defineImmutable
//
// This should be invoked by a derived class.
//-------------------------------------------------------------------------------------------
void GenomeSchema::defineImmutable()
{
#define __CONST(NAME, VAL) add( new ImmutableScalarGene(NAME, VAL) )
#define CONSTANT(NAME, VAL) __CONST(#NAME, VAL)
#define RANGE(NAME,MIN,MAX) __CONST(#NAME"_min",MIN); __CONST(#NAME"_max",MAX)
#define INTERPOLATED(NAME,MIN,MAX) add( new ImmutableInterpolatedGene(#NAME, \
																	  __CONST(#NAME"_min",MIN),	\
																	  __CONST(#NAME"_max",MAX), \
																	  __InterpolatedGene::ROUND_INT_NEAREST) )
	RANGE( ID, 0.0, 1.0 );
	RANGE( Size,
		   agent::config.minAgentSize,
		   agent::config.maxAgentSize );
	RANGE( MutationRate,
		   GenomeSchema::config.minMutationRate,
		   GenomeSchema::config.maxMutationRate );
	RANGE( CrossoverPointCount,
		   GenomeSchema::config.minNumCpts,
		   GenomeSchema::config.maxNumCpts );
	RANGE( LifeSpan,
		   agent::config.minLifeSpan,
		   agent::config.maxLifeSpan );
	RANGE( Strength,
		   agent::config.minStrength,
		   agent::config.maxStrength );
	RANGE( MaxSpeed,
		   agent::config.minmaxspeed,
		   agent::config.maxmaxspeed );
	RANGE( MateEnergyFraction,
		   agent::config.minmateenergy,
		   agent::config.maxmateenergy );
	assert( Metabolism::getNumberOfDefinitions() > 0 );
	if( Metabolism::selectionMode == Metabolism::Gene 
		&& Metabolism::getNumberOfDefinitions() > 1 )
	{
		RANGE( MetabolismIndex,
			   0,
			   Metabolism::getNumberOfDefinitions() - 1 );
	}
	CONSTANT( MiscBias,
			  GenomeSchema::config.miscBias );
	CONSTANT( MiscInvisSlope,
			  GenomeSchema::config.miscInvisSlope );
	INTERPOLATED( BitProbability,
				  GenomeSchema::config.minBitProb,
				  GenomeSchema::config.maxBitProb );
	CONSTANT( GrayCoding,
			  GenomeSchema::config.grayCoding );
	RANGE( LearningRate,
		   Brain::config.minlrate,
		   Brain::config.maxlrate );	
	RANGE( Bias,
		   -Brain::config.maxbias,
		   Brain::config.maxbias );
	if( Brain::config.neuronModel == Brain::Configuration::TAU )
	{
		RANGE( Tau,
			   Brain::config.Tau.minVal,
			   Brain::config.Tau.maxVal );
	}
	if( Brain::config.neuronModel == Brain::Configuration::SPIKING )
	{
		RANGE( ScaleLatestSpikes,
			   0.1,
			   0.9 );
		if( Brain::config.Spiking.enableGenes == true) {
			RANGE( SpikingParameterA,
				   Brain::config.Spiking.aMinVal,
				   Brain::config.Spiking.aMaxVal );
			RANGE( SpikingParameterB,
				   Brain::config.Spiking.bMinVal,
				   Brain::config.Spiking.bMaxVal );
			RANGE( SpikingParameterC,
				   Brain::config.Spiking.cMinVal,
				   Brain::config.Spiking.cMaxVal );
			RANGE( SpikingParameterD,
				   Brain::config.Spiking.dMinVal,
				   Brain::config.Spiking.dMaxVal );
		}
	}

	CONSTANT( maxsynapse2energy,
			  Brain::config.maxsynapse2energy );
	CONSTANT( maxneuron2energy,
			  Brain::config.maxneuron2energy );

#undef __CONST
#undef CONSTANT
#undef RANGE
#undef INTERPOLATED
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::defineMutable
//
// This should be invoked by a derived class.
//-------------------------------------------------------------------------------------------
void GenomeSchema::defineMutable()
{
#define SCALAR(NAME) add( new MutableScalarGene(#NAME,					\
												get(#NAME"_min"),		\
												get(#NAME"_max"),		\
												__InterpolatedGene::ROUND_INT_FLOOR) )
#define INDEX(NAME) add( new MutableScalarGene(#NAME,					\
											   get(#NAME"_min"),		\
											   get(#NAME"_max"),		\
											   __InterpolatedGene::ROUND_INT_BIN) )
	SCALAR( MutationRate );

	SCALAR( CrossoverPointCount );
	SCALAR( LifeSpan );
	SCALAR( ID );
	SCALAR( Strength );
	SCALAR( Size );
	SCALAR( MaxSpeed );
	SCALAR( MateEnergyFraction );
	if( Metabolism::selectionMode == Metabolism::Gene
		&& Metabolism::getNumberOfDefinitions() > 1 )
	{
		INDEX( MetabolismIndex );
	}

	if( Brain::config.neuronModel == Brain::Configuration::SPIKING )
	{
		SCALAR( ScaleLatestSpikes );
	}

#undef SCALAR
#undef INDEX
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::seed
//
// This should be invoked by a derived class.
//-------------------------------------------------------------------------------------------
void GenomeSchema::seed( Genome *g )
{

#define SEED(NAME,VAL) g->seed( get(#NAME), VAL)

	// ---
	// --- SCALARS (e.g. MutationRate, LifeSpan, Size)
	// --- 
	citfor( GeneVector, getAll(GeneType::SCALAR), it )
	{
		Gene *gene = *it;
		if( gene->ismutable )
		{
			g->seed( gene, 0.5 );
		}
	}

	SEED( MutationRate, GenomeSchema::config.seedMutationRate );

	if( Metabolism::selectionMode == Metabolism::Gene
		&& Metabolism::getNumberOfDefinitions() > 1 )
	{
		SEED( MetabolismIndex, randpw() );
	}
#undef SEED
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::add
//-------------------------------------------------------------------------------------------
Gene *GenomeSchema::add( Gene *gene )
{
	assert( state == STATE_CONSTRUCTING );
	assert( gene->schema == NULL );
	assert( get(gene->name) == NULL );

	gene->schema = this;

	name2gene[gene->name] = gene;
	type2genes[gene->type].push_back( gene );
	genes.push_back( gene );

	return gene;
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::get
//-------------------------------------------------------------------------------------------
Gene *GenomeSchema::get( const string &name )
{
	return name2gene[name];
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::get
//-------------------------------------------------------------------------------------------
Gene *GenomeSchema::get( const char *name )
{
	return name2gene[name];
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::get
//-------------------------------------------------------------------------------------------
Gene *GenomeSchema::get( const GeneType *type )
{
	GeneVector &genes = type2genes[type];

	assert( genes.size() == 1 );

	return genes.front();
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::getAll
//-------------------------------------------------------------------------------------------
const GeneVector &GenomeSchema::getAll( const GeneType *type )
{
	return type2genes[type];
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::getGeneCount
//-------------------------------------------------------------------------------------------
int GenomeSchema::getGeneCount( const GeneType *type )
{
	return type2genes[type].size();
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::getPhysicalCount
//-------------------------------------------------------------------------------------------
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
				continue;

			if( g->type == GeneType::SCALAR )
				n++;
			else
				return cache.physicalCount = n;
		}

		assert(false); // fell through!
		return -1;
	}
	default:
		assert(false);
	}
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::getMutableSize
//-------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------
// GenomeSchema::beginComplete
//-------------------------------------------------------------------------------------------
void GenomeSchema::beginComplete()
{
	assert(state == STATE_CONSTRUCTING);

	state = STATE_CACHING;
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::endComplete
//-------------------------------------------------------------------------------------------
void GenomeSchema::endComplete()
{
	assert( state == STATE_CACHING );

	getPhysicalCount();
	getMutableSize();

	state = STATE_COMPLETE;
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::getIndexes
//-------------------------------------------------------------------------------------------
void GenomeSchema::getIndexes( vector<string> &geneNames, vector<int> &result )
{
	itfor( vector<string>, geneNames, it )
	{
		Gene *gene = name2gene[*it];
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
// GenomeSchema::printIndexes
//-------------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------------
// GenomeSchema::printTitles
//-------------------------------------------------------------------------------------------
void GenomeSchema::printTitles( FILE *f )
{
	itfor( GeneList, genes, it )
	{
		(*it)->printTitles( f );
	}
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::printRanges
//-------------------------------------------------------------------------------------------
void GenomeSchema::printRanges( FILE *f )
{
	itfor( GeneList, genes, it )
	{
		(*it)->printRanges( f );
	}
}
