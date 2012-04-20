#include "FiringRateModel.h"

#include "debug.h"
#include "Genome.h"
#include "GenomeSchema.h"
#include "misc.h"

#include "Brain.h" // temporary

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

void FiringRateModel::init_derived( float initial_activation )
{

	for( int i = 0; i < dims->numneurons; i++ )
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

	n.tau = attrs->tau;
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
        
        for( i = dims->firstNonInputNeuron; i < dims->numneurons; i++ )
        {
            for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
            {
				printf("%3d   %3d    %3d    %5ld    %f\n",
					   i, synapse[k].toneuron, synapse[k].fromneuron,
					   k, synapse[k].efficacy); 
            }
        }
	)


	for( i = dims->firstOutputNeuron; i < dims->firstInternalNeuron; i++ )
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
		if( Brain::config.neuronModel == Brain::Configuration::TAU )
		{
			float tau = neuron[i].tau;
			newneuronactivation[i] = (1.0 - tau) * neuronactivation[i]  +  tau * logistic( newneuronactivation[i], Brain::config.logisticSlope );
		}
		else
		{
			newneuronactivation[i] = logistic( newneuronactivation[i], Brain::config.logisticSlope );
		}
	#endif
	}

	long numneurons = dims->numneurons;
	float logisticSlope = Brain::config.logisticSlope;
    for( i = dims->firstInternalNeuron; i < numneurons; i++ )
    {
		float newactivation = neuron[i].bias;
        for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
        {
            newactivation += synapse[k].efficacy *
               neuronactivation[synapse[k].fromneuron];
		}
        //newneuronactivation[i] = logistic(newneuronactivation[i], Brain::config.logisticSlope);

		if( Brain::config.neuronModel == Brain::Configuration::TAU )
		{
			float tau = neuron[i].tau;
			newactivation = (1.0 - tau) * neuronactivation[i]  +  tau * logistic( newactivation, logisticSlope );
		}
		else
		{
			newactivation = logistic( newactivation, logisticSlope );
		}

        newneuronactivation[i] = newactivation;
    }

#define DebugBrain 0
#if DebugBrain
	static int numDebugBrains = 0;
	if( (TSimulation::fAge > 1) && (numDebugBrains < 10) )
	{
		numDebugBrains++;
		printf( "***** age = %ld *****\n", TSimulation::fAge );
		printf( "random neuron [%d] = %g\n", randomneuron, neuronactivation[randomneuron] );
		printf( "energy neuron [%d] = %g\n", energyneuron, neuronactivation[energyneuron] );
		printf( "red neurons (%d):\n", fNumRedNeurons );
		for( i = 0; i < fNumRedNeurons; i++ )
			printf( "    %3d  %2d  %g\n", i+redneuron, i, neuronactivation[i+redneuron] );
		printf( "green neurons (%d):\n", fNumGreenNeurons );
		for( i = 0; i < fNumGreenNeurons; i++ )
			printf( "    %3d  %2d  %g\n", i+greenneuron, i, neuronactivation[i+greenneuron] );
		printf( "blue neurons (%d):\n", fNumBlueNeurons );
		for( i = 0; i < fNumBlueNeurons; i++ )
			printf( "    %3d  %2d  %g\n", i+blueneuron, i, neuronactivation[i+blueneuron] );
		printf( "output neurons (%d):\n", dims->numOutputNeurons );
		for( i = dims->firstOutputNeuron; i < dims->firstInternalNeuron; i++ )
		{
			printf( "    %3d  %g  %g synapses (%ld):\n", i, neuron[i].bias, newneuronactivation[i], neuron[i].endsynapses - neuron[i].startsynapses );
			for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
				printf( "        %4ld  %2ld  %2d  %g  %g\n", k, k-neuron[i].startsynapses, synapse[k].fromneuron, synapse[k].efficacy, neuronactivation[synapse[k].fromneuron] );
		}
		printf( "internal neurons (%d):\n", numnoninputneurons-dims->numOutputNeurons );
		for( i = dims->firstInternalNeuron; i < dims->numneurons; i++ )
		{
			printf( "    %3d  %g  %g synapses (%ld):\n", i, neuron[i].bias, newneuronactivation[i], neuron[i].endsynapses - neuron[i].startsynapses );
			for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
				printf( "        %4ld  %2ld  %2d  %g  %g\n", k, k-neuron[i].startsynapses, synapse[k].fromneuron, synapse[k].efficacy, neuronactivation[synapse[k].fromneuron] );
		}
	}
#endif

    debugcheck( "after updating neurons" );

	IF_BPRINT
	(
        printf("  i neuron[i].bias neuronactivation[i] newneuronactivation[i]\n");
        for (i = 0; i < dims->numneurons; i++)
            printf( "%3d  %1.4f  %1.4f  %1.4f\n", i, neuron[i].bias, neuronactivation[i], newneuronactivation[i] );
	)

//	printf( "yaw activation = %g\n", newneuronactivation[yawneuron] );

    float learningrate;
	long numsynapses = dims->numsynapses;
    for (k = 0; k < numsynapses; k++)
    {
		FiringRateModel__Synapse &syn = synapse[k];

		learningrate = syn.lrate;

		float efficacy = syn.efficacy + learningrate
			* (newneuronactivation[syn.toneuron]-0.5f)
			* (   neuronactivation[syn.fromneuron]-0.5f);

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

    debugcheck( "after updating synapses" );

    float* saveneuronactivation = neuronactivation;
    neuronactivation = newneuronactivation;
    newneuronactivation = saveneuronactivation;
}
