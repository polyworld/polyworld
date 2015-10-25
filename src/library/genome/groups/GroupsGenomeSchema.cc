#include "GroupsGenomeSchema.h"

#include "GroupsGenome.h"
#include "agent/agent.h"
#include "brain/groups/GroupsBrain.h"
#include "sim/globals.h"

using namespace genome;

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::GroupsGenomeSchema
//-------------------------------------------------------------------------------------------
GroupsGenomeSchema::GroupsGenomeSchema()
: GenomeSchema()
{
#define SYNAPSE_TYPE(NAME) \
	{\
		GroupsSynapseType *st = new GroupsSynapseType( this, #NAME, synapseTypes.size() ); \
		synapseTypes.push_back( st );\
		synapseTypeMap[#NAME] = st;\
	}

	SYNAPSE_TYPE( EE );
	SYNAPSE_TYPE( EI );
	SYNAPSE_TYPE( II );
	SYNAPSE_TYPE( IE );

#undef SYNAPSE_TYPE
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::~GroupsGenomeSchema
//-------------------------------------------------------------------------------------------
GroupsGenomeSchema::~GroupsGenomeSchema()
{
	for( GroupsSynapseTypeList::iterator
			 it = synapseTypes.begin(),
			 end = synapseTypes.end();
		 it != end;
		 it++ )
	{
		delete (*it);
	}
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::define
//-------------------------------------------------------------------------------------------
void GroupsGenomeSchema::define()
{
	// ---
	// --- Base Class
	// ---
	GenomeSchema::define();

#define INPUT1(NAME) add( new ImmutableNeurGroupGene(#NAME,			\
													 NGT_INPUT) )
#define INPUT(NAME, MINNEUR, MAXNEUR) add( new MutableNeurGroupGene(#NAME, \
																	NGT_INPUT, \
																	MINNEUR, \
																	MAXNEUR) )
#define OUTPUT(NAME) add( new ImmutableNeurGroupGene(#NAME,			\
													 NGT_OUTPUT) )
#define INTERNAL(NAME, MINNEUR, MAXNEUR) add( new MutableNeurGroupGene(#NAME, \
																	   NGT_INTERNAL, \
																	   MINNEUR, \
																	   MAXNEUR) )
#define GROUP_ATTR(NAME, GROUP, MINVAL, MAXVAL) add( new NeurGroupAttrGene(#NAME, \
																		   NGT_##GROUP,	\
																		   MINVAL, \
																		   MAXVAL) )
#define SYNAPSE_ATTR(NAME,NEGATE_I,LESS_THAN_ZERO, MINVAL, MAXVAL) add( new SynapseAttrGene(#NAME, \
																							NEGATE_I, \
																							LESS_THAN_ZERO, \
																							MINVAL, \
																							MAXVAL) )

	INPUT1( Random );
	INPUT1( Energy );
	if( agent::config.enableMateWaitFeedback )
		INPUT1( MateWaitFeedback );
	if( agent::config.enableSpeedFeedback )
		INPUT1( SpeedFeedback );
	if( agent::config.enableCarry )
	{
		INPUT1( Carrying );
		INPUT1( BeingCarried );
	}
	INPUT( Red,
		   GroupsBrain::config.minvisneurpergroup,
		   GroupsBrain::config.maxvisneurpergroup );
	INPUT( Green,
		   GroupsBrain::config.minvisneurpergroup,
		   GroupsBrain::config.maxvisneurpergroup );
	INPUT( Blue,
		   GroupsBrain::config.minvisneurpergroup,
		   GroupsBrain::config.maxvisneurpergroup );

	OUTPUT( Eat );
	OUTPUT( Mate );
	OUTPUT( Fight );
	OUTPUT( Speed );
	OUTPUT( Yaw );
	if( agent::config.yawEncoding == agent::YE_OPPOSE )
		OUTPUT( YawOppose );
	OUTPUT( Light );
	OUTPUT( Focus );

	if( agent::config.enableVisionPitch )
		OUTPUT( VisionPitch );

	if( agent::config.enableVisionYaw )
		OUTPUT( VisionYaw );

	if( agent::config.enableGive )
		OUTPUT( Give );

	if( agent::config.enableCarry )
	{
		OUTPUT( Pickup );
		OUTPUT( Drop );
	}

	INTERNAL( InternalNeuronGroupCount,
			  GroupsBrain::config.mininternalneurgroups,
			  GroupsBrain::config.maxinternalneurgroups );

	GROUP_ATTR( ExcitatoryNeuronCount,
				INTERNAL,
				GroupsBrain::config.mineneurpergroup,
				GroupsBrain::config.maxeneurpergroup );

	GROUP_ATTR( InhibitoryNeuronCount,
				INTERNAL,
				GroupsBrain::config.minineurpergroup,
				GroupsBrain::config.maxineurpergroup );
				
	GROUP_ATTR( Bias,
				NONINPUT,
				-Brain::config.maxbias,
				Brain::config.maxbias );
				
	if( Brain::config.neuronModel == Brain::Configuration::TAU )
	{
		GROUP_ATTR( Tau,
					NONINPUT,
					Brain::config.Tau.minVal,
					Brain::config.Tau.maxVal );
	}

	if( Brain::config.neuronModel == Brain::Configuration::SPIKING
		&& Brain::config.Spiking.enableGenes == true )
	{
		GROUP_ATTR( SpikingParameterA,
					NONINPUT,
					Brain::config.Spiking.aMinVal,
					Brain::config.Spiking.aMaxVal );

		GROUP_ATTR( SpikingParameterB,
					NONINPUT,
					Brain::config.Spiking.bMinVal,
					Brain::config.Spiking.bMaxVal );

		GROUP_ATTR( SpikingParameterC,
					NONINPUT,
					Brain::config.Spiking.cMinVal,
					Brain::config.Spiking.cMaxVal );

		GROUP_ATTR( SpikingParameterD,
					NONINPUT,
					Brain::config.Spiking.dMinVal,
					Brain::config.Spiking.dMaxVal );
	}

	SYNAPSE_ATTR( ConnectionDensity,
				  false,
				  false,
				  GroupsBrain::config.minconnectiondensity,
				  GroupsBrain::config.maxconnectiondensity );

	SYNAPSE_ATTR( LearningRate,
				  true,
				  true,
				  Brain::config.minlrate,
				  Brain::config.maxlrate );	

	SYNAPSE_ATTR( TopologicalDistortion,
				  false,
				  false,
				  GroupsBrain::config.mintopologicaldistortion,
				  GroupsBrain::config.maxtopologicaldistortion );
				  
	if( GroupsBrain::config.enableTopologicalDistortionRngSeed )
	{
		SYNAPSE_ATTR( TopologicalDistortionRngSeed,
					  false,
					  false,
					  GroupsBrain::config.minTopologicalDistortionRngSeed,
					  GroupsBrain::config.maxTopologicalDistortionRngSeed );

	}
	if( GroupsBrain::config.enableInitWeightRngSeed )
	{
		SYNAPSE_ATTR( InitWeightRngSeed,
					  false,
					  false,
					  GroupsBrain::config.minInitWeightRngSeed,
					  GroupsBrain::config.maxInitWeightRngSeed );
	}
	
#undef INPUT1
#undef INPUT
#undef OUTPUT
#undef INTERNAL
#undef GROUP_ATTR
#undef SYNAPSE_ATTR
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::seed
//-------------------------------------------------------------------------------------------
void GroupsGenomeSchema::seed( Genome *g_ )
{
	if( GenomeSchema::config.simpleSeed )
	{
		g_->randomize();
	}

	// ---
	// --- Base Class
	// ---
	GenomeSchema::seed( g_ );

	GroupsGenome *g = dynamic_cast<GroupsGenome *>( g_ );

#define SEED(NAME,VAL) g->seed( get(#NAME), VAL)
#define SEED_GROUP(NAME,GROUP,VAL)				\
	g->seed( get(#NAME),						\
			 get(#GROUP),						\
			 VAL )
#define SEED_SYNAPSE(NAME,TYPE,FROM,TO,VAL)		\
	g->seed( get(#NAME),						\
			 getSynapseType(#TYPE),				\
			 get(#FROM),						\
			 get(#TO),							\
			 VAL )

	SEED( Red, 0.5 );
	SEED( Green, 0.5 );
	SEED( Blue, 0.5 );
	SEED( InternalNeuronGroupCount, 0 );

	if( GenomeSchema::config.simpleSeed )
	{
		return;
	}

	SEED( ExcitatoryNeuronCount, 0 );
	SEED( InhibitoryNeuronCount, 0 );

	SEED( Bias, 0.5 );
	if( Brain::config.neuronModel == Brain::Configuration::TAU )
	{
		SEED( Tau, Brain::config.Tau.seedVal );
	}

	SEED_GROUP( Bias, Mate, 1.0 );
	SEED_GROUP( Bias, Fight, GenomeSchema::config.seedFightBias );
	if( agent::config.enableGive )
	{
		SEED_GROUP( Bias, Give, GenomeSchema::config.seedGiveBias );
	}
	if( agent::config.enableCarry )
	{
		SEED_GROUP( Bias, Pickup, GenomeSchema::config.seedPickupBias );
		SEED_GROUP( Bias, Drop, GenomeSchema::config.seedDropBias );
	}

	SEED( ConnectionDensity, 0 );
	SEED( LearningRate, 0 );
	SEED( TopologicalDistortion, 0 );
	if( GroupsBrain::config.enableTopologicalDistortionRngSeed )
	{
		SEED( TopologicalDistortionRngSeed, 0 );
	}
	if( GroupsBrain::config.enableInitWeightRngSeed )
	{
		SEED( InitWeightRngSeed, 0 );
	}

	SEED_SYNAPSE( ConnectionDensity,	 EE, Red,   Fight,	GenomeSchema::config.seedFightExcitation );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Green, Eat,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Blue,  Mate,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Red,   Speed,	0.5 );
	SEED_SYNAPSE( TopologicalDistortion, IE, Red,   Speed,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Green, Speed,	0.5 );
	SEED_SYNAPSE( TopologicalDistortion, EE, Green, Speed,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Blue,  Speed,	0.5 );
	SEED_SYNAPSE( TopologicalDistortion, EE, Blue,  Speed,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Red,   Yaw,	0.5 );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Red,   Yaw,	0.5 );
	SEED_SYNAPSE( TopologicalDistortion, IE, Red,   Yaw,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Green, Yaw,	0.5 );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Green, Yaw,	0.5 );
	SEED_SYNAPSE( TopologicalDistortion, IE, Green, Yaw,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 EE, Blue,  Yaw,	0.5 );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Blue,  Yaw,	0.5 );
	SEED_SYNAPSE( TopologicalDistortion, IE, Blue,  Yaw,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Eat,   Fight,	GenomeSchema::config.seedFightExcitation );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Mate,	Fight,	GenomeSchema::config.seedFightExcitation );
	if( agent::config.enableCarry )
	{
		SEED_SYNAPSE( ConnectionDensity,	 EE, Red,	Pickup,	GenomeSchema::config.seedPickupExcitation );
		SEED_SYNAPSE( ConnectionDensity,	 EE, Green,	Pickup,	GenomeSchema::config.seedPickupExcitation );
		SEED_SYNAPSE( ConnectionDensity,	 EE, Blue,	Pickup,	GenomeSchema::config.seedPickupExcitation );
		SEED_SYNAPSE( ConnectionDensity,	 EE, Red,	Drop,	GenomeSchema::config.seedDropExcitation );
		SEED_SYNAPSE( ConnectionDensity,	 EE, Green,	Drop,	GenomeSchema::config.seedDropExcitation );
		SEED_SYNAPSE( ConnectionDensity,	 EE, Blue,	Drop,	GenomeSchema::config.seedDropExcitation );
		SEED_SYNAPSE( ConnectionDensity,	 IE, Pickup,Drop,	GenomeSchema::config.seedPickupExcitation ); // if wiring in pickup, have it suppress drop
	}

#undef SEED
#undef SEED_GROUP
#undef SEED_SYNAPSE
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::createGenome
//-------------------------------------------------------------------------------------------
Genome *GroupsGenomeSchema::createGenome( GenomeLayout *layout )
{
	return new GroupsGenome( this, layout );
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::add
//-------------------------------------------------------------------------------------------
Gene *GroupsGenomeSchema::add( Gene *gene )
{
	gene = GenomeSchema::add( gene );

	if( gene->type == GroupsGeneType::NEURGROUP )
	{
		neurgroups.push_back( gene );
		GroupsGeneType::to_NeurGroup(gene)->schema = this;
	}
	else if( gene->type == GroupsGeneType::NEURGROUP_ATTR )
	{
		GroupsGeneType::to_NeurGroupAttr(gene)->schema = this;
	}
	else if( gene->type == GroupsGeneType::SYNAPSE_ATTR )
	{
		GroupsGeneType::to_SynapseAttr(gene)->schema = this;
	}

	return gene;
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getPhysicalCount
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getPhysicalCount()
{
	switch(_state)
	{
	case STATE_COMPLETE:
		return cache.physicalCount;
	case STATE_CACHING: {
		int n = 0;

		itfor( GeneVector, _genes, it )
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
// GroupsGenomeSchema::getMaxGroupCount
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getMaxGroupCount( NeurGroupType group )
{
	switch(_state)
	{
	case STATE_COMPLETE:
		return cache.groupCount[group];
	case STATE_CACHING: {
		int n = 0;

		itfor( GeneVector, neurgroups, it )
		{
			NeurGroupGene *gene = GroupsGeneType::to_NeurGroup(*it);

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

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getFirstGroup
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getFirstGroup( Gene *_gene )
{
	NeurGroupGene *gene = GroupsGeneType::to_NeurGroup(_gene);

	switch(_state)
	{
	case STATE_COMPLETE:
		return gene->first_group;
	case STATE_CACHING: {
		int group = 0;

		itfor( GeneVector, neurgroups, it )
		{
			NeurGroupGene *other = GroupsGeneType::to_NeurGroup(*it);
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

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getFirstGroup
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getFirstGroup( NeurGroupType group )
{
	switch(_state)
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

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getGroupGene
//-------------------------------------------------------------------------------------------
NeurGroupGene *GroupsGenomeSchema::getGroupGene( int group )
{
	Gene *gene;

	switch( getNeurGroupType(group) )
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
		gene = _type2genes[GroupsGeneType::NEURGROUP][group];
		break;
	case NGT_INTERNAL:
		gene = _type2genes[GroupsGeneType::NEURGROUP].back();
		break;
	default:
		assert(false);
	}

	return GroupsGeneType::to_NeurGroup(gene);
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getMaxNeuronCount
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getMaxNeuronCount( NeurGroupType group )
{
	switch(_state)
	{
	case STATE_COMPLETE:
		return cache.neuronCount[group];
	case STATE_CACHING: {
		int n = 0;

		itfor( GeneVector, neurgroups, it )
		{
			NeurGroupGene *gene = GroupsGeneType::to_NeurGroup(*it);

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

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getNeurGroupType
//-------------------------------------------------------------------------------------------
NeurGroupType GroupsGenomeSchema::getNeurGroupType( int group )
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


//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getSynapseTypeCount
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getSynapseTypeCount()
{
	return synapseTypes.size();
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getSynamseTypes
//-------------------------------------------------------------------------------------------
const GroupsSynapseTypeList &GroupsGenomeSchema::getSynapseTypes()
{
	return synapseTypes;
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getSynapseType
//-------------------------------------------------------------------------------------------
GroupsSynapseType *GroupsGenomeSchema::getSynapseType( const char *name )
{
	return synapseTypeMap[name];
}

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::getMaxSynapseCount
//-------------------------------------------------------------------------------------------
int GroupsGenomeSchema::getMaxSynapseCount()
{
	switch(_state)
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

//-------------------------------------------------------------------------------------------
// GroupsGenomeSchema::complete
//-------------------------------------------------------------------------------------------
void GroupsGenomeSchema::complete( int offset )
{
	beginComplete( offset );

	getPhysicalCount();

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

	itfor( GeneVector, neurgroups, it )
	{
		Gene *gene = *it;

		getFirstGroup(gene);
	}

	for( GroupsSynapseTypeList::iterator
			 it = synapseTypes.begin(),
			 end = synapseTypes.end();
		 it != end;
		 it++ )
	{
		(*it)->complete( cache.groupCount );
	}

	endComplete();
}
