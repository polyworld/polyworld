#include "FiringRateModel.h"

#include "genome/Genome.h"
#include "genome/GenomeSchema.h"
#include "sim/debug.h"
#include "utils/misc.h"


using namespace std;

using namespace genome;


#define GaussianOutputNeurons 0
#if GaussianOutputNeurons
	#define GaussianActivationMean 0.0
	#define GaussianActivationStandardDeviation 1.5
	#define GaussianActivationVariance (GaussianActivationStandardDeviation * GaussianActivationStandardDeviation)
#endif


FiringRateModel::FiringRateModel( NervousSystem *cns )
: BaseNeuronModel<Neuron, NeuronAttrs, Synapse>( cns )
{
}

FiringRateModel::~FiringRateModel()
{
}

void FiringRateModel::init_derived( double initial_activation )
{

	for( int i = 0; i < dims->numNeurons; i++ )
		neuronactivation[i] = initial_activation;
}

void FiringRateModel::set_neuron( int index,
								  void *attributes,
								  int startsynapses,
								  int endsynapses )
{
	BaseNeuronModel<Neuron, NeuronAttrs, Synapse>::set_neuron( index,
															   attributes,
															   startsynapses,
															   endsynapses );
	NeuronAttrs *attrs = (NeuronAttrs *)attributes;
	Neuron &n = neuron[index];

	assert( !isnan(attrs->tau) );
	assert( !isnan(attrs->gain) );
	n.tau = attrs->tau;
	n.gain = attrs->gain;
}

void FiringRateModel::update( bool bprint )
{
    debugcheck( "(firing-rate brain) on entry" );

    short i;
    long k;
    if ((neuron == NULL) || (synapse == NULL) || (neuronactivation == NULL))
        return;

	IF_BPRINTED
	(
        printf("neuron (toneuron)  fromneuron   synapse   efficacy\n");
        
        for( i = dims->getFirstOutputNeuron(); i < dims->numNeurons; i++ )
        {
            for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
            {
				printf("%3d   %3d    %3d    %5ld    %f\n",
					   i, synapse[k].toneuron, synapse[k].fromneuron,
					   k, synapse[k].efficacy); 
            }
        }
	)


	for( i = 0; i < dims->getFirstOutputNeuron(); i++ )
	{
		newneuronactivation[i] = neuronactivation[i];
	}

	for( i = dims->getFirstOutputNeuron(); i < dims->getFirstInternalNeuron(); i++ )
	{
        newneuronactivation[i] = neuron[i].bias;
        for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
        {
            newneuronactivation[i] += synapse[k].efficacy *
               neuronactivation[synapse[k].fromneuron];
		}              
	#if GaussianOutputNeurons
        newneuronactivation[i] = gaussian( newneuronactivation[i], GaussianActivationMean, GaussianActivationVariance );
	#else
		if( Brain::config.neuronModel == Brain::Configuration::TAU_GAIN )
		{
			float tau = neuron[i].tau;
			float gain = neuron[i].gain;
			newneuronactivation[i] = (1.0 - tau) * neuronactivation[i]  +  tau * logistic( newneuronactivation[i], gain );
		}
		else
		{
			newneuronactivation[i] = logistic( newneuronactivation[i], Brain::config.logisticSlope );
		}
	#endif
	}

	long numneurons = dims->numNeurons;
	float logisticSlope = Brain::config.logisticSlope;
    for( i = dims->getFirstInternalNeuron(); i < numneurons; i++ )
    {
		double newactivation = neuron[i].bias;
        for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
        {
            newactivation += synapse[k].efficacy *
               neuronactivation[synapse[k].fromneuron];
		}
        //newneuronactivation[i] = logistic(newneuronactivation[i], Brain::config.logisticSlope);

		if( Brain::config.neuronModel == Brain::Configuration::TAU_GAIN )
		{
			float tau = neuron[i].tau;
			float gain = neuron[i].gain;
			newactivation = (1.0 - tau) * neuronactivation[i]  +  tau * logistic( newactivation, gain );
		}
		else
		{
			newactivation = logistic( newactivation, logisticSlope );
		}

        newneuronactivation[i] = newactivation;
    }

    debugcheck( "after updating neurons" );

	IF_BPRINT
	(
        printf("  i neuron[i].bias neuronactivation[i] newneuronactivation[i]\n");
        for (i = 0; i < dims->numNeurons; i++)
            printf( "%3d  %1.4f  %1.4f  %1.4f\n", i, neuron[i].bias, neuronactivation[i], newneuronactivation[i] );
	)

//	printf( "yaw activation = %g\n", newneuronactivation[yawneuron] );

    if (Brain::config.enableLearning && !cns->getBrain()->isFrozen())
    {
        float learningrate;
		long numsynapses = dims->numSynapses;
        for (k = 0; k < numsynapses; k++)
        {
			FiringRateModel__Synapse &syn = synapse[k];

			learningrate = syn.lrate;

			float efficacy = syn.efficacy + learningrate
				* (newneuronactivation[syn.toneuron]-0.5)
				* (   neuronactivation[syn.fromneuron]-0.5);

            if (fabs(efficacy) > (0.5f * Brain::config.maxWeight))
            {
                efficacy *= 1.0f - (1.0f - Brain::config.decayRate) *
                    (fabs(efficacy) - 0.5f * Brain::config.maxWeight) / (0.5f * Brain::config.maxWeight);
                if (efficacy > Brain::config.maxWeight)
                    efficacy = Brain::config.maxWeight;
                else if (efficacy < -Brain::config.maxWeight)
                    efficacy = -Brain::config.maxWeight;
            }
            else
            {
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
                // not strictly correct for this to be in an else clause,
                // but if lrate is reasonable, efficacy should never change
                // sign with a new magnitude greater than 0.5 * Brain::config.maxWeight
                if (learningrate >= 0.0f)  // excitatory
                    efficacy = MAX(0.0f, efficacy);
                if (learningrate < 0.0f)  // inhibitory
                    efficacy = MIN(-1.e-10f, efficacy);
            }

			syn.efficacy = efficacy;
        }
    }

    debugcheck( "after updating synapses" );

    double* saveneuronactivation = neuronactivation;
    neuronactivation = newneuronactivation;
    newneuronactivation = saveneuronactivation;
}
