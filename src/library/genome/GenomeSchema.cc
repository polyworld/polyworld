#include "GenomeSchema.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "agent/agent.h"
#include "agent/Metabolism.h"
#include "genome/sheets/SheetsGenomeSchema.h"
#include "utils/misc.h"

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
		if( layout == "NeurGroup")
			GenomeSchema::config.layoutType = genome::GenomeLayout::NeurGroup;
		else if( layout == "None" )
			GenomeSchema::config.layoutType = genome::GenomeLayout::None;
		else
			assert( false );
	}
	{
		string resolution = doc.get( "GeneticOperatorResolution" );
		if( resolution == "Bit" )
			GenomeSchema::config.resolution = GenomeSchema::RESOLUTION_BIT;
		else if( resolution == "Byte" )
			GenomeSchema::config.resolution = GenomeSchema::RESOLUTION_BYTE;
		else
			assert( false );
	}
    GenomeSchema::config.dieAtMaxAge = doc.get( "DieAtMaxAge" );
    GenomeSchema::config.enableEvolution = doc.get( "EnableEvolution" );
    GenomeSchema::config.minMutationRate = doc.get( "MinMutationRate" );
    GenomeSchema::config.maxMutationRate = doc.get( "MaxMutationRate" );
    GenomeSchema::config.minMutationStdevPower = doc.get( "MinMutationStdevPower" );
    GenomeSchema::config.maxMutationStdevPower = doc.get( "MaxMutationStdevPower" );
    GenomeSchema::config.minNumCpts = doc.get( "MinCrossoverPoints" );
    GenomeSchema::config.maxNumCpts = doc.get( "MaxCrossoverPoints" );
    GenomeSchema::config.minLifeSpan = doc.get( "MinLifeSpan" );
    GenomeSchema::config.maxLifeSpan = doc.get( "MaxLifeSpan" );
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
	{
		string seedType = doc.get( "SeedType" );
		if( seedType == "Legacy" )
			GenomeSchema::config.seedType = GenomeSchema::SEED_LEGACY;
		else if( seedType == "Simple" )
			GenomeSchema::config.seedType = GenomeSchema::SEED_SIMPLE;
		else if( seedType == "Random" )
			GenomeSchema::config.seedType = GenomeSchema::SEED_RANDOM;
		else
			assert( false );
	}
	GenomeSchema::config.seedMutationRate = doc.get( "SeedMutationRate" );
	GenomeSchema::config.simpleSeedYawBiasDelta = doc.get( "SimpleSeedYawBiasDelta" );
	GenomeSchema::config.seedFightBias = doc.get( "SeedFightBias" );
	GenomeSchema::config.seedFightExcitation = doc.get( "SeedFightExcitation" );
	GenomeSchema::config.seedGiveBias = doc.get( "SeedGiveBias" );
	GenomeSchema::config.seedPickupBias = doc.get( "SeedPickupBias" );
	GenomeSchema::config.seedDropBias = doc.get( "SeedDropBias" );
	GenomeSchema::config.seedPickupExcitation = doc.get( "SeedPickupExcitation" );
	GenomeSchema::config.seedDropExcitation = doc.get( "SeedDropExcitation" );
    GenomeSchema::config.grayCoding = doc.get( "GrayCoding" );


	SheetsGenomeSchema::processWorldfile( doc );
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::GenomeSchema
//-------------------------------------------------------------------------------------------
GenomeSchema::GenomeSchema()
: GeneSchema()
{
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::~GenomeSchema
//-------------------------------------------------------------------------------------------
GenomeSchema::~GenomeSchema()
{
}

//-------------------------------------------------------------------------------------------
// GenomeSchema::define
//
// This should be invoked by a derived class.
//-------------------------------------------------------------------------------------------
void GenomeSchema::define()
{
#define INTERPOLATED_IMMUTABLE(NAME,MIN,MAX) add( new ImmutableInterpolatedGene(#NAME, \
																				MIN, \
																				MAX, \
																				__InterpolatedGene::ROUND_INT_NEAREST) )
#define SCALAR(NAME, MINVAL, MAXVAL) if( MINVAL == MAXVAL ) \
									 add( new ImmutableScalarGene(#NAME, MINVAL) ); \
									 else \
									 add( new MutableScalarGene(#NAME, MINVAL, MAXVAL, __InterpolatedGene::ROUND_INT_FLOOR) )
#define INDEX(NAME, MINVAL, MAXVAL) add( new MutableScalarGene(#NAME,	\
															   MINVAL,	\
															   MAXVAL,	\
															   __InterpolatedGene::ROUND_INT_BIN) )

	INTERPOLATED_IMMUTABLE( BitProbability,
							GenomeSchema::config.minBitProb,
							GenomeSchema::config.maxBitProb );

	SCALAR( MutationRate,
			GenomeSchema::config.minMutationRate,
			GenomeSchema::config.maxMutationRate );

	if( GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BYTE )
	{
		SCALAR( MutationStdevPower,
				GenomeSchema::config.minMutationStdevPower,
				GenomeSchema::config.maxMutationStdevPower );
	}

	SCALAR( CrossoverPointCount,
		   GenomeSchema::config.minNumCpts,
		   GenomeSchema::config.maxNumCpts );

	if( GenomeSchema::config.dieAtMaxAge )
	{
		SCALAR( LifeSpan,
			   GenomeSchema::config.minLifeSpan,
			   GenomeSchema::config.maxLifeSpan );
	}

	if( agent::config.bodyGreenChannel == agent::BGC_ID )
	{
		SCALAR( ID,
				0.0,
				1.0 );
	}

	SCALAR( Strength,
		   agent::config.minStrength,
		   agent::config.maxStrength );

	SCALAR( Size,
		   agent::config.minAgentSize,
		   agent::config.maxAgentSize );

	SCALAR( MaxSpeed,
		   agent::config.minmaxspeed,
		   agent::config.maxmaxspeed );

	SCALAR( MateEnergyFraction,
		   agent::config.minmateenergy,
		   agent::config.maxmateenergy );
			
	if( Metabolism::selectionMode == Metabolism::Gene
		&& (Metabolism::getNumberOfDefinitions() > 1) )
	{
		INDEX( MetabolismIndex,
			   0,
			   Metabolism::getNumberOfDefinitions() - 1 );
	}

	if( Brain::config.neuronModel == Brain::Configuration::SPIKING )
	{
		SCALAR( ScaleLatestSpikes,
			   0.1,
			   0.9 );
	}

#undef INTERPOLATED_IMMUTABLE
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

	if( GenomeSchema::config.minMutationRate != GenomeSchema::config.maxMutationRate )
	{
		SEED( MutationRate, GenomeSchema::config.seedMutationRate );
	}

	if( Metabolism::selectionMode == Metabolism::Gene
		&& Metabolism::getNumberOfDefinitions() > 1 )
	{
		SEED( MetabolismIndex, randpw() );
	}
#undef SEED
}
