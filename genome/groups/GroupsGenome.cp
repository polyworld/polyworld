#include "GroupsGenome.h"

#include "globals.h"
#include "GroupsBrain.h"
#include "GroupsGenomeSchema.h"

using namespace genome;

GroupsGenome::GroupsGenome( GroupsGenomeSchema *schema,
							GenomeLayout *layout )
: Genome( schema,
		  layout )
, _schema( schema )
{
	CONNECTION_DENSITY = gene("ConnectionDensity");
	TOPOLOGICAL_DISTORTION = gene("TopologicalDistortion");
	LEARNING_RATE = gene("LearningRate");
	INHIBITORY_COUNT = gene("InhibitoryNeuronCount");
	EXCITATORY_COUNT = gene("ExcitatoryNeuronCount");
	BIAS = gene("Bias");
	INTERNAL = schema->getGroupGene( schema->getFirstGroup(NGT_INTERNAL) );

	if( Brain::config.neuronModel == Brain::Configuration::TAU )
		TAU = gene( "Tau" );
	else
		TAU = NULL;

	if( (Brain::config.neuronModel == Brain::Configuration::SPIKING)
		&& Brain::config.Spiking.enableGenes )
	{
		SPIKING_A = gene("SpikingParameterA");
		SPIKING_B = gene("SpikingParameterB");
		SPIKING_C = gene("SpikingParameterC");
		SPIKING_D = gene("SpikingParameterD");
	}
	else
	{
		SPIKING_A = NULL;
		SPIKING_B = NULL;
		SPIKING_C = NULL;
		SPIKING_D = NULL;
	}

#define ST(NAME) NAME = schema->getSynapseType(#NAME)
	ST(EE);
	ST(EI);
	ST(IE);
	ST(II);
#undef ST
}

GroupsGenome::~GroupsGenome()
{
}

Brain *GroupsGenome::createBrain( NervousSystem *cns )
{
	return new GroupsBrain( cns, this );
}

GroupsGenomeSchema *GroupsGenome::getSchema()
{
	return _schema;
}

int GroupsGenome::getGroupCount( NeurGroupType type )
{
	if( (type == NGT_INPUT) || (type == NGT_OUTPUT) )
	{
		return _schema->getMaxGroupCount( type );
	}
	else
	{
		int noninternal;

		switch( type )
		{
		case NGT_ANY:
			noninternal = _schema->getMaxGroupCount( NGT_INPUT )
				+ _schema->getMaxGroupCount( NGT_OUTPUT );
			break;
		case NGT_INTERNAL:
			noninternal = 0;
			break;
		case NGT_NONINPUT:
			noninternal = _schema->getMaxGroupCount( NGT_OUTPUT );
			break;
		default:
			assert(false);
		}

		return noninternal + get( INTERNAL );
	}
}

int GroupsGenome::getNeuronCount( NeuronType type,
								  int group )
{
	NeurGroupGene *g = _schema->getGroupGene( group );

	switch( g->getGroupType() )
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
		return get( g );
	case NGT_INTERNAL:
		switch(type)
		{
		case INHIBITORY:
			return get( INHIBITORY_COUNT,
						group );
		case EXCITATORY:
			return get( EXCITATORY_COUNT,
						group );
		default:
			assert(false);
		}
	default:
		assert(false);
	}
}

int GroupsGenome::getNeuronCount( int group )
{
	switch( _schema->getNeurGroupType(group) )
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
		return getNeuronCount( INHIBITORY,
							   group );
	case NGT_INTERNAL:
		return getNeuronCount( INHIBITORY,
							   group ) 
			+ getNeuronCount( EXCITATORY,
							  group );
	default:
		assert(false);
	}
}

const GroupsSynapseTypeList &GroupsGenome::getSynapseTypes()
{
	return _schema->getSynapseTypes();
}

int GroupsGenome::getSynapseCount( GroupsSynapseType *synapseType,
								   int from,
								   int to )
{
	NeuronType nt_from = synapseType->nt_from;
	NeuronType nt_to = synapseType->nt_to;
	bool to_output = _schema->getNeurGroupType(to) == NGT_OUTPUT;

	if( (nt_to == INHIBITORY) && to_output )
	{
		// As targets, the output neurons are treated exclusively as excitatory
		return 0;
	}

	float cd = get( CONNECTION_DENSITY,
					synapseType,
					from,
					to );

	int nfrom = getNeuronCount( nt_from, from );
	int nto = getNeuronCount( nt_to, to );

	if( from == to )
	{
		if( (synapseType == IE) && to_output )
		{
			// If the source and target groups are the same, and both are an output
			// group, then this will evaluate to zero.
			nto--;
		}
		else if( nt_from == nt_to )
		{
			nto--;
		}
	}

	return nint(cd * nfrom * nto);
}

int GroupsGenome::getSynapseCount( int from,
								   int to )
{
	int n = 0;

	for( GroupsSynapseTypeList::const_iterator
			 it = _schema->getSynapseTypes().begin(),
			 end = _schema->getSynapseTypes().end();
		 it != end;
		 it++ )
	{
		n += getSynapseCount( *it, from, to );
	}

	return n;
}

Scalar GroupsGenome::get( Gene *gene,
						  int group )
{
	return GroupsGeneType::to_NeurGroupAttr(gene)->get( this,
														group );
}

Scalar GroupsGenome::get( Gene *gene,
						  GroupsSynapseType *synapseType,
						  int from,
						  int to )
{
	return GroupsGeneType::to_SynapseAttr(gene)->get( this,
													  synapseType,
													  from,
													  to );
}

#define SEEDCHECK(VAL) assert(((VAL) >= 0) && ((VAL) <= 1))
#define SEEDVAL(VAL) (unsigned char)((VAL) * 255)

void GroupsGenome::seed( Gene *attr,
						 Gene *group,
						 float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	GroupsGeneType::to_NeurGroupAttr(attr)->seed( this,
												  GroupsGeneType::to_NeurGroup(group),
												  SEEDVAL(rawval_ratio) );
}

void GroupsGenome::seed( Gene *gene,
						 GroupsSynapseType *synapseType,
						 Gene *from,
						 Gene *to,
						 float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	GroupsGeneType::to_SynapseAttr(gene)->seed( this,
												synapseType,
												GroupsGeneType::to_NeurGroup(from),
												GroupsGeneType::to_NeurGroup(to),
												SEEDVAL(rawval_ratio) );
}

void GroupsGenome::getCrossoverPoints( long *crossoverPoints, long numCrossPoints )
{
	long numphysbytes = _schema->getPhysicalCount();

    // guarantee crossover in "physiology" genes
    crossoverPoints[0] = long(randpw() * numphysbytes * 8 - 1);
    crossoverPoints[1] = numphysbytes * 8;

	// Generate & order the crossover points.
	// Start iteration at [2], as [0], [1] set above
    long i, j;
    
    for (i = 2; i <= numCrossPoints; i++) 
    {
        long newCrossPoint = long(randpw() * (nbytes - numphysbytes) * 8 - 1) + crossoverPoints[1];
        bool equal;
        do
        {
            equal = false;
            for (j = 1; j < i; j++)
            {
                if (newCrossPoint == crossoverPoints[j])
                    equal = true;
            }
            
            if (equal)
                newCrossPoint = long(randpw() * (nbytes - numphysbytes) * 8 - 1) + crossoverPoints[1];
                
        } while (equal);
        
        if (newCrossPoint > crossoverPoints[i-1])
        {
            crossoverPoints[i] = newCrossPoint;  // happened to come out ordered
		}           
        else
        {
            for (j = 2; j < i; j++)
            {
                if (newCrossPoint < crossoverPoints[j])
                {
                    for (long k = i; k > j; k--)
                        crossoverPoints[k] = crossoverPoints[k-1];
                    crossoverPoints[j] = newCrossPoint;
                    break;
                }
            }
        }
    }
}
