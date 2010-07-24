#include "GenomeUtil.h"

#include <assert.h>

#include "agent.h"
#include "Brain.h"
#include "Gene.h"
#include "globals.h"

using namespace genome;

GenomeSchema *GenomeUtil::schema = NULL;


GenomeSchema *GenomeUtil::createSchema()
{
	assert(schema == NULL);

	schema = new GenomeSchema();

	// ---
	// --- Immutable Genes
	// ---
#define __CONST(NAME, VAL) schema->add( new ImmutableScalarGene(NAME, VAL) )
#define CONSTANT(NAME, VAL) __CONST(#NAME, VAL)
#define RANGE(NAME,MIN,MAX) __CONST(#NAME"_min",MIN); __CONST(#NAME"_max",MAX)
#define INTERPOLATED(NAME,MIN,MAX) schema->add( new ImmutableInterpolatedGene(#NAME, \
																			  __CONST(#NAME"_min",MIN),	\
																			  __CONST(#NAME"_max",MAX)) )
	RANGE( ID, 0.0, 1.0 );
	RANGE( Size,
		   agent::gMinAgentSize,
		   agent::gMaxAgentSize );
	RANGE( MutationRate,
		   genome::gMinMutationRate,
		   genome::gMaxMutationRate );
	RANGE( CrossoverPointCount,
		   genome::gMinNumCpts,
		   genome::gMaxNumCpts );
	RANGE( LifeSpan,
		   genome::gMinLifeSpan,
		   genome::gMaxLifeSpan );
	RANGE( Strength,
		   genome::gMinStrength,
		   genome::gMaxStrength );
	RANGE( MaxSpeed,
		   genome::gMinmaxspeed,
		   genome::gMaxmaxspeed );
	RANGE( MateEnergyFraction,
		   genome::gMinmateenergy,
		   genome::gMaxmateenergy );
	CONSTANT( MiscBias,
			  genome::gMiscBias );
	CONSTANT( MiscInvisSlope,
			  genome::gMiscInvisSlope );
	INTERPOLATED( BitProbability,
				  genome::gMinBitProb,
				  genome::gMaxBitProb );
	CONSTANT( GrayCoding,
			  genome::gGrayCoding );
	RANGE( VisionNeuronCount,
		   genome::gMinvispixels,
		   genome::gMaxvispixels );
	RANGE( LearningRate,
		   genome::gMinlrate,
		   genome::gMaxlrate );
	RANGE( InternalNeuronGroupCount,
		   brain::gNeuralValues.mininternalneurgroups,
		   brain::gNeuralValues.maxinternalneurgroups );
	RANGE( ExcitatoryNeuronCount,
		   brain::gNeuralValues.mineneurpergroup,
		   brain::gNeuralValues.maxeneurpergroup );
	RANGE( InhibitoryNeuronCount,
		   brain::gNeuralValues.minineurpergroup,
		   brain::gNeuralValues.maxineurpergroup );
	RANGE( Bias,
		   -brain::gNeuralValues.maxbias,
		   brain::gNeuralValues.maxbias );
	RANGE( BiasLearningRate,
		   brain::gNeuralValues.minbiaslrate,
		   brain::gNeuralValues.maxbiaslrate );
	RANGE( ConnectionDensity,
		   brain::gNeuralValues.minconnectiondensity,
		   brain::gNeuralValues.maxconnectiondensity );
	RANGE( TopologicalDistortion,
		   brain::gNeuralValues.mintopologicaldistortion,
		   brain::gNeuralValues.maxtopologicaldistortion );
	CONSTANT( maxsynapse2energy,
			  brain::gNeuralValues.maxsynapse2energy );
	CONSTANT( maxneuron2energy,
			  brain::gNeuralValues.maxneuron2energy );

#undef __CONST
#undef CONSTANT
#undef RANGE

	// ---
	// --- Mutable & Input/Output Genes
	// ---
#define SCALAR(NAME) schema->add( new MutableScalarGene(#NAME,			\
														schema->get(#NAME"_min"), \
														schema->get(#NAME"_max")) )
#define INPUT1(NAME) schema->add( new ImmutableNeurGroupGene(#NAME,		\
															 NGT_INPUT) )
#define INPUT(NAME, RANGE_NAME) schema->add( new MutableNeurGroupGene(#NAME, \
																	  NGT_INPUT, \
																	  schema->get(#RANGE_NAME"_min"), \
																	  schema->get(#RANGE_NAME"_max")) )
#define OUTPUT(NAME) schema->add( new ImmutableNeurGroupGene(#NAME,		\
															 NGT_OUTPUT) )
#define INTERNAL(NAME) schema->add( new MutableNeurGroupGene(#NAME,		\
															 NGT_INTERNAL, \
															 schema->get(#NAME"_min"), \
															 schema->get(#NAME"_max")) )
#define GROUP_ATTR(NAME, GROUP) schema->add( new NeurGroupAttrGene(#NAME,\
																   NGT_##GROUP,	\
																   schema->get(#NAME"_min"),\
																   schema->get(#NAME"_max")) )
#define SYNAPSE_ATTR(NAME,NEGATE_I,LESS_THAN_ZERO) schema->add( new SynapseAttrGene(#NAME, \
																					NEGATE_I, \
																					LESS_THAN_ZERO, \
																					schema->get(#NAME"_min"), \
																					schema->get(#NAME"_max")) )
	SCALAR( MutationRate );

	SCALAR( CrossoverPointCount );
	SCALAR( LifeSpan );
	SCALAR( ID );
	SCALAR( Strength );
	SCALAR( Size );
	SCALAR( MaxSpeed );
	SCALAR( MateEnergyFraction );

	INPUT1( Random );
	INPUT1( Energy );
	if( genome::gEnableSpeedFeedback )
		INPUT1( SpeedFeedback );
	INPUT( Red, VisionNeuronCount );
	INPUT( Green, VisionNeuronCount );
	INPUT( Blue, VisionNeuronCount );

	OUTPUT( Eat );
	OUTPUT( Mate );
	OUTPUT( Fight );
	OUTPUT( Speed );
	OUTPUT( Yaw );
	OUTPUT( Light );
	OUTPUT( Focus );

	if( genome::gEnableGive )
		OUTPUT( Give );

	INTERNAL( InternalNeuronGroupCount );

	GROUP_ATTR( ExcitatoryNeuronCount, INTERNAL );
	GROUP_ATTR( InhibitoryNeuronCount, INTERNAL );
	GROUP_ATTR( Bias, NONINPUT );
	GROUP_ATTR( BiasLearningRate, NONINPUT );

	SYNAPSE_ATTR( ConnectionDensity, false, false );
	SYNAPSE_ATTR( LearningRate, true, true );
	SYNAPSE_ATTR( TopologicalDistortion, false, false );

#undef SCALAR
#undef INPUT1
#undef INPUT
#undef OUTPUT
#undef INTERNAL
#undef GROUP_ATTR
#undef SYNAPSE_ATTR

	// ---
	// --- Finalization
	// ---
	schema->complete();	

	return schema;
}

Genome *GenomeUtil::createGenome( bool randomized )
{
	assert(schema);

	Genome *g = new Genome( schema );

	if( randomized )
	{
		randomize( g );
	}
	
	return g;
}

void GenomeUtil::randomize( Genome *g )
{
#if DesignerGenes
	seed( g );
#else
	g->randomize();
#endif
}

void GenomeUtil::seed( Genome *g )
{
	assert( schema );
	assert( g );

#define SEED(NAME,VAL) g->seed( schema->get(#NAME), VAL)
#define SEED_GROUP(NAME,GROUP,VAL)				\
	g->seed( schema->get(#NAME),				\
			 schema->get(#GROUP),				\
			 VAL )
#define SEED_SYNAPSE(NAME,TYPE,FROM,TO,VAL)		\
	g->seed( schema->get(#NAME),				\
			 schema->getSynapseType(#TYPE),		\
			 schema->get(#FROM),				\
			 schema->get(#TO),					\
			 VAL )

	SEED( MutationRate, 0.5 );
	SEED( CrossoverPointCount, 0.5 );

	SEED( LifeSpan, 0.5 );
	SEED( ID, 0.5 );
	SEED( Strength, 0.5 );
	SEED( Size, 0.5 );
	SEED( MaxSpeed, 0.5 );
	SEED( MateEnergyFraction, 0.5 );

	SEED( Red, 0.5 );
	SEED( Green, 0.5 );
	SEED( Blue, 0.5 );
	SEED( InternalNeuronGroupCount, 0 );

	SEED( ExcitatoryNeuronCount, 0 );
	SEED( InhibitoryNeuronCount, 0 );

	SEED( Bias, 0.5 );
	SEED( BiasLearningRate, 0 );

	SEED_GROUP( Bias, Mate, 1.0 );

	SEED( ConnectionDensity, 0 );
	SEED( LearningRate, 0 );
	SEED( TopologicalDistortion, 0 );

	SEED_SYNAPSE( ConnectionDensity,	 EE, Red,   Fight,	1.0 );
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
	SEED_SYNAPSE( ConnectionDensity,	 IE, Eat,   Fight,	1.0 );
	SEED_SYNAPSE( ConnectionDensity,	 IE, Mate,	Fight,	1.0 );

#undef SEED
#undef SEED_GROUP
#undef SEED_SYNAPSE
}

#include <fstream>

using namespace std;

void GenomeUtil::test()
{
	GenomeSchema &schema = *createSchema();

	// ---
	// --- Schema Diagnostics
	// ---
	FILE *f = fopen("Genome-index.txt", "w");
	schema.printIndexes( f );
	fclose(f);

	cout << "--- Genome ---" << endl;
	
	cout << "ninput = " << schema.getMaxGroupCount( NGT_INPUT ) << endl;
	cout << "noutput = " << schema.getMaxGroupCount( NGT_OUTPUT ) << endl;
	cout << "nnoninput = " << schema.getMaxGroupCount( NGT_NONINPUT ) << endl;
	cout << "maxinternalgroups = " << schema.getMaxGroupCount( NGT_INTERNAL ) << endl;
	cout << "maxneurgroups = " << schema.getMaxGroupCount( NGT_ANY ) << endl;
	cout << endl;
	
	cout << "maxinputneurons = " << schema.getMaxNeuronCount( NGT_INPUT ) << endl;
	cout << "maxinternalneurons = " << schema.getMaxNeuronCount( NGT_INTERNAL ) << endl;
	cout << "maxnoninputneurons = " << schema.getMaxNeuronCount( NGT_NONINPUT ) << endl;
	cout << "maxneurons = " << schema.getMaxNeuronCount( NGT_ANY ) << endl;
	cout << endl;

	cout << "maxsynapses = " << schema.getMaxSynapseCount() << endl;
	cout << endl;

	cout << "Genome datasize = " << schema.getMutableSize() << endl;
	cout << endl;

	// ---
	// --- Seed Genome Instance
	// ---
	Genome *genome = new Genome( &schema );
	GenomeUtil::seed( genome );

	ofstream out("Genome-dump.txt");
	genome->dump(out);

	// ---
	// --- 
	// ---
	srand48(0);

	Genome *g1 = new Genome( &schema );
	g1->randomize();
	ofstream out1("Genome1-dump.txt");
	g1->dump(out1);

	Genome *g2 = new Genome( &schema );
	g2->randomize();
	ofstream out2("Genome2-dump.txt");

	out2 << "CrossoverPointCount=" << (int)g2->get("CrossoverPointCount") << endl;
	out2 << "MutationRate=" << (float)g2->get("MutationRate") << endl;
	out2 << "PhysicalCount=" << schema.getPhysicalCount() << endl;

	g2->dump(out2);

	Genome *g3 = new Genome( &schema );
	g3->crossover( g1, g2, true );
	ofstream out3("Genome3-dump.txt");
	g3->dump(out3);

	Genome *g4 = new Genome( &schema );
	g4->copyFrom( g3 );
	g4->mutate();
	ofstream out4("Genome4-dump.txt");
	g4->dump(out4);

	ofstream out5("Genome-all.txt");

	out5 << "MutationRate=" << (float)g4->get( "MutationRate" ) << endl;
	out5 << "CrossoverPoints=" << (int)g4->get( "CrossoverPointCount" ) << endl;
	out5 << "Lifespan=" << (int)g4->get( "LifeSpan" ) << endl;
	out5 << "ID=" << (float)g4->get( "ID" ) << endl;
	out5 << "Strength=" << (float)g4->get( "Strength" ) << endl;
	out5 << "MaxSpeed=" << (float)g4->get( "MaxSpeed" ) << endl;
	out5 << "MateEnergy=" << (float)g4->get( "MateEnergyFraction" ) << endl;
	out5 << "NumNeuronGroups=" << (int)g4->getGroupCount(NGT_ANY) << endl;	

	out5 << "maxbias=" << (float)g4->get("Bias_max") << ",-maxbias=" << (float)g4->get("Bias_min") << endl;

	int n = g4->getGroupCount(NGT_ANY);
	for(int i = 0; i < n; i++)
		//for(int i = 2; i < 3; i++)
	{
		out5 << "- Group " << i;
		out5 << ",numeneur=" << (int)g4->getNeuronCount(EXCITATORY, i);
		out5 << ",numineur=" << (int)g4->getNeuronCount(INHIBITORY, i);
		out5 << ",numneurons=" << (int)g4->getNeuronCount(i);
		out5 << ",Bias=" << (float)g4->get( g4->gene("Bias"), i );
		out5 << endl;
	}

	Gene *cd = schema.get("ConnectionDensity");
	Gene *td = schema.get("TopologicalDistortion");
	Gene *lr = schema.get("LearningRate");

	for(int from = 0; from < n; from++)
	{
		for(int to = 5; to < n; to++)
		{
			out5 << "- Synapse " << from << "->" << to;
			out5 << ",eecd=" << (float)g4->get( cd, g4->EE, from, to );
			out5 << ",eicd=" << (float)g4->get( cd, g4->EI, from, to );
			out5 << ",iicd=" << (float)g4->get( cd, g4->II, from, to );
			out5 << ",iecd=" << (float)g4->get( cd, g4->IE, from, to );
			out5 << ",eetd=" << (float)g4->get( td, g4->EE, from, to );
			out5 << ",eitd=" << (float)g4->get( td, g4->EI, from, to );
			out5 << ",iitd=" << (float)g4->get( td, g4->II, from, to );
			out5 << ",ietd=" << (float)g4->get( td, g4->IE, from, to );
			out5 << ",eelr=" << (float)g4->get( lr, g4->EE, from, to );
			out5 << ",eilr=" << (float)g4->get( lr, g4->EI, from, to );
			out5 << ",iilr=" << (float)g4->get( lr, g4->II, from, to );
			out5 << ",ielr=" << (float)g4->get( lr, g4->IE, from, to );
			out5 << ",neesyn=" << g4->getSynapseCount( g4->EE, from, to );
			out5 << ",neisyn=" << g4->getSynapseCount( g4->EI, from, to );
			out5 << ",niisyn=" << g4->getSynapseCount( g4->II, from, to );
			out5 << ",niesyn=" << g4->getSynapseCount( g4->IE, from, to );
			out5 << ",nsyn=" << g4->getSynapseCount(from, to);
			out5 << endl;
		}
	}
	out5 << endl;

	out5 << "separation(1,2)=" << (float)g1->separation( g2 ) << endl;
	out5 << "separation(1,4)=" << (float)g1->separation( g4 ) << endl;
	out5 << "mateProbability(1,2)=" << g1->mateProbability( g2 ) << endl;
	out5 << "mateProbability(1,4)=" << g1->mateProbability( g4 ) << endl;	
}
