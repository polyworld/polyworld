#include "SpikingModel.h"

#include <assert.h>
#include <stdlib.h>

#include "debug.h"
#include "Genome.h"
#include "GenomeSchema.h"
#include "misc.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"

#include "Brain.h" // temporary

using namespace std;


SpikingModel::SpikingModel( NervousSystem *cns, float scale_latest_spikes_ )
: BaseNeuronModel<Neuron, NeuronAttrs, Synapse>( cns )
, scale_latest_spikes( scale_latest_spikes_ )
{
	this->rng = cns->getRNG();

	outputActivation = NULL;
}

SpikingModel::~SpikingModel()
{
	free( outputActivation );
}

void SpikingModel::init_derived( float initial_activation )
{
#define ALLOC(NAME, TYPE, N) if(NAME) free(NAME); NAME = (TYPE *)calloc(N, sizeof(TYPE)); assert(NAME);

	ALLOC( outputActivation, float, dims->numOutputNeurons );

#undef ALLOC

	// TODO: initial_activation is currently ignored for backwards-compatibility
	for( int i = 0; i < dims->numNeurons; i++ )
	{		
		neuronactivation[i] = SpikingActivation; 
	}		
	
	for( int i = 0; i < dims->numOutputNeurons; i++ )
	{
		outputActivation[i]= 0.0;	// fmax((double)neuron[acc+dims->getFirstOutputNeuron()].bias / brain::Brain::config.maxbias, 0.0);
	}
}

void SpikingModel::set_neuron( int index,
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

	n.SpikingParameter_a = attrs->SpikingParameter_a;
	n.SpikingParameter_b = attrs->SpikingParameter_b;
	n.SpikingParameter_c = attrs->SpikingParameter_c;
	n.SpikingParameter_d = attrs->SpikingParameter_d;
	n.v = -70;
	n.u = -14;
	n.maxfiringcount = 1;
}

void SpikingModel::update( bool bprint )
{
	FILE *fHandle = NULL;

	unsigned char spikeMatrix[dims->numNeurons][BrainStepsPerWorldStep];  //assuming BrainStepsPerWorldStep = 100
	for (int j=0; j <  dims->numNeurons; j++)
		for (int i = 0; i< BrainStepsPerWorldStep; i++)
			spikeMatrix[j][i] = '0';
	
    debugcheck( "(spiking brain) on entry" );
	float inputFiringProbability[dims->numInputNeurons];
	static long loop_counter=0;
    short i, n_steps;
    long k;
// 	float u;
	float v,activation;
    if ((neuron == NULL) || (synapse == NULL) || (neuronactivation == NULL))
        return;
	int outputNeuronFiringCounter[dims->numOutputNeurons];
	loop_counter++;
	short NeuronFiringCounter[dims->numNeurons];
	
#if IraDebug	
	//???????????????????????????????????????????????????????????????????????????????
	//debug stuff	
	if (agentID<4)
		printf("agent %d entering update\n", agentID);
	else
	{
		for (i = 0; i < dims->numOutputNeurons; i++)
			neuronactivation[i+dims->getFirstOutputNeuron()]=0;
		neuronactivation[yawneuron]=.5;	
		return;
	}	
	//???????????????????????????????????????????????????????????????????????????????
#endif
	

	for (i = 0; i < dims->numNeurons; i++)
		NeuronFiringCounter[i]=0;						//since we start a new brain step reset firing counter
		
	//Further down in the code I turn output neuron activation into firing rates.  This has to be done because
	//the rest of polyworld expects, roughly firing rates from output neurons.  However the outputneurons in 
	//polyworld have connections to other neurons in polyworld and therefore their activation level should adhere 
	//to the constant SpikingActivation which the non output neurons all adhere to.  This way we all spiking
	//activtions in the brain update are uniform.
	for (i = 0; i < dims->numOutputNeurons; i++)
	{
		outputNeuronFiringCounter[i]=0;
		if(neuron[i+dims->getFirstOutputNeuron()].v>=30)	
			neuronactivation[i+dims->getFirstOutputNeuron()]=SpikingActivation;//previously had this as one....eeek!
		else
			neuronactivation[i+dims->getFirstOutputNeuron()]=0;
	}

	for( i = 0; i < dims->numInputNeurons; i++ )
	{
		inputFiringProbability[i] = neuronactivation[i];
		neuronactivation[i] = 0;
	}
	
	//######################################################################################################################
	//INNER BRAIN
	//This is the code for the inner neuron's activation calculations and update rules for synapses onto
	//each neuron.
	//Note I treat newneuronactivation as input to Izhikevich's voltage equasions
	//######################################################################################################################
	//scale the bias learning rate
    float* saveneuronactivation = neuronactivation;
	
	//calculate activity in each neuron i then learn for each synapse in i
	long synapsesToDepress[dims->numSynapses];
	int numSynapsesToDepress = 0, startSynapsesToDepress = 0;
	short fromNeuron = 0;
	for (n_steps = 0; n_steps < BrainStepsPerWorldStep; n_steps++)
	{		
		numSynapsesToDepress = startSynapsesToDepress = 0;
		//determine if the input neuron stochastically fires
		//not that the newneuronactivation must contain the threshold calculated earlier thats why we have saveneuronactivation
		
		//now here I scan though the list of input neurons.  They should have a firing probability,  see inputFiringProbability above,
		//that we can generate a random number, check against that random number to see if the inputFiringProbability is less than the 
		//number, if so we have exeded the probability theshold and can force the neuron to fire.  Otherwise force the activation to zero.
		for (i = 0; i < dims->getFirstOutputNeuron(); i++)
		{
//			printf("input firing prob = %f\n", inputFiringProbability[i]);
			if( rng->drand() < inputFiringProbability[i])
			{
			
				spikeMatrix[i][n_steps] = '1'; //This is strictly for debug printout purposes
				newneuronactivation[i] = SpikingActivation;
				NeuronFiringCounter[i]++;
				neuron[i].v=31;              //hack for stdp
			}

			else
			{
				newneuronactivation[i] = 0.0;
				neuron[i].v= -30; //or any value less than 30 for that matter
			}	
		}

		//here we itterate though the non-input neurons
		for (i = dims->getFirstOutputNeuron(); i < dims->numNeurons; i++)
		{
			//The bias seemed to be negatively affecting timing in out learning rule which corelates timing with 
			//how stronly you learned.  Forcing the neuron to spike based on bias could cause associations that have 
			//nothing to do with the the incoming synapses causing the neuron to fire, but rather the bias causing the
			//nueron to fire and all incoming sysnapses would be attenuated or bolstered whether they helped to cause
			//a action potential or not.
			newneuronactivation[i] = .0;

			startSynapsesToDepress = numSynapsesToDepress;
	        //cycle through each synapse k for a given neuron i
			for (k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++)
			{
				fromNeuron = (short)abs(synapse[k].fromneuron);  //grab a synapse's from neuron
				activation = neuronactivation[fromNeuron];		 //grab its activation		
				//see if the synapse is active if so add it's value to the activation and location to the container
				if ( activation )  //see if its active
				{        
					newneuronactivation[i] += synapse[k].efficacy * activation; //add it to the neuron's overall activation
					synapsesToDepress[numSynapsesToDepress++]=k;  //need this for stdp learning rule
				}       

			}
			
			v = neuron[i].v;                        //get the current membrane potential
		
			//test to see if a spike had previously occured. If it did set a flag saying it just spiked
			//then reset the membrane potential and recovery variable. 	
			if (v>=30.)
			{										  
				neuron[i].v =  neuron[i].SpikingParameter_c;  //reset the membrane potential
				neuron[i].u += neuron[i].SpikingParameter_d;  //reset the recovery variable
				v = neuron[i].v;			
			}	
				
// 			u=neuron[i].u;
			
#if USE_BIAS			
			//stochastically generate bias
			if (rng->drand() < (1.0 / (1.0 + exp(-1 * neuron[i].bias * .5))))
				newneuronactivation[i] += BIAS_INJECTED_VOLTAGE;
#endif
			
			//Calculate Izhikevich's formula for voltage.  I don't understand why he does it twice, but 
			//I have learned not to mess with things in he does that you don't understand.  Obviously there
			//is a reason he does it this way.
			
			neuron[i].v = v + (.5 * ((0.04 * v * v) + (5 * v) + 140-neuron[i].u + newneuronactivation[i]));
//change later all v should be neuron[i].v otherwise it is the same assignment as above....dummy!
//			neuron[i].v = v + (.5 * ((0.04 * v * v) + (5 * v) + 140-neuron[i].u + newneuronactivation[i]));
			neuron[i].u += neuron[i].SpikingParameter_a * (neuron[i].SpikingParameter_b * v - neuron[i].u);
					
			//##############################################################################################################
			//If the membrane potetial is high enough that means an action potential will be generated.  Here we have a 
			//high enough voltage to generate an action potential.  This means that all the synapses (i or e) that 
			//contributed to the firing of the neuron are to be updated.  Here, in a generic Hebbian fasion we itterate 
			//through a list of synapses that just fired and stenghten each connection in that list, based on each synapses 
			//individual learing rate.
			//##############################################################################################################
			if(neuron[i].v >= 30.)
			{
				numSynapsesToDepress = startSynapsesToDepress;			//since we fired there is no need to depress
				spikeMatrix[i][n_steps] = '1';							//printout matrix gets a one for a spike
				NeuronFiringCounter[i]++;								//we fired thus we increment
				if(i < dims->getFirstInternalNeuron() && i >= dims->getFirstOutputNeuron())	//see if it's an output neuron
					outputNeuronFiringCounter[i - dims->getFirstOutputNeuron()]++; //keep track of the total number of spike for output firing rate
				newneuronactivation[i]=SpikingActivation;               //v>30 means a firing!
				


				/*The learning algorithm here has 2 steps
				
				This is step one.
				The neuron has fired thus it rewards every incoming conection.  If the fromNeuron 
				has recently fired it's stdp will be proportionally large to it's temporal difference
				from when the toNeuron fires, which it fires now.  The leads into somewhat of a debate
				because neurons keep getting rewarded if this neuron keeps firing, and they shouldn't
				because when the toNeuron resets, the current that caused it to fire is disipated and 
				thus has no role in causing the neuron to fire again in other time steps.  On way to 
				around this is to set up two STDPs on for potentiation and one for depression, then clear
				each one when learning takes place.  But thats one more float for each synapse in each 
				brain in each agent and I have made the spiking neurons end of the code bulky enought.
				Here is an example how learning takes place when Z fires.
					
				Step 1                Step 2                  Step 3                 Step 4
				STDP multipled by     STDP *= .95,			  STDP *= .95,           STDP *= .95, 
				.95.                  B & C fire.             B & C's STDPs reset	 B's STDPs reset
				A fires               A's STDP reset to .1    B fires again          Potentiation takes place
				A(0.0)-^->             A(0.1)--->             A(.095)--->             A(.090)--->
				B(0.0)--->  Z(0.0)     B(0.0)-^->  Z(0.0)     B(0.1)-^->  Z(0.0)      B(0.1)---->  Z(0.0)-^->
				C(0.0)--->             C(0.0)-^->             C(0.1)--->              C(.095)--->
			
				Keep in mind that we are only demontrating learning for Z.  On step four since z has fired
				potentiation must take place.  Supposing that A B and C are all the synapses that connect to
				Z we simply itterate through all synapes and potentiate each synapses delta by the STDP of 
				the from neuron.  Thus in step four synpase(A-->Z).delta will be incremented by .09, 
				synpase(B-->Z).delta by .1 and synpase(A-->Z).delta by .09.  At the end of a brain step the 
				deltas will come back into play.
				*/
				
				//this took a while to spot, or rather took larry going "oh it's a delta" and me "huh?".  
				//what Izhikevich does in his code is to give an STDP value to each synapse.  					
					    
				for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )			
					synapse[k].delta += neuron[(int)abs(synapse[k].fromneuron)].STDP;	//I have fired reward all my incoming conections
			
			}
			// there is no spike thus default activation for the neuron is 0	
			else
				newneuronactivation[i] = 0.;  
		}

		/*
		
		Step 1                Step 2                  Step 3                 Step 4
	    STDP multipled by     STDP *= .95,			  STDP *= .95,           STDP *= .95, 
		.95.                  Z again fires           nobody fires nothing	 Z fires as does A and B
		Z fires, helps B      none other fires        happens              
		fire
		      -^-> A(0.02)            -^-> A(0.019)             ---> A(.018)           -^->A(.1)
		Z(0.0)-^-> B(0.1)       Z(0.1)-^-> B(0.95)      Z(0.095)---> B(0.09)    Z(0.09)-^->B(0.1)
		      -^-> C(.095)            -^->  C(0.09)             ---> C(.095)           -^-> C(.81)
	
		Alright this sequence is very unlikely to happen most likely Z will only fire 10 times a brain step here it 
		fires 3 to demonstrate what I feel is a problem of the algorithm.  Step z fires.  Synapse(Z-A) is depressed
		by .02, synapse(Z-B) is depressed because B just fired and it is handled as a special case, however we have a 
		huge depression for synapse(Z-C) because Z fired right after C previously fired which is what we want.
		Step 2 Synapse(Z-A) -= .019 synapse(Z-B) -= .95, and synapse(Z-C) -= .09.  Step 3 nothing happens.  
		synapse(Z-A) and synapse(Z-B) are not depressed because of both A and B firing.  My doubt again come into play
		here, in this extremely rare situation, synapse(Z-C) is repeatedly being punsished by C never firing...on the
		other hand that could be exactly how our neurons work, I really don't know.  Notice here that neuron Z's STDP
		never affects the depression calculations.   
		*/			
		float stdp = 0;
		Synapse * syn;		
		k=0;
		for(i = 0; i < numSynapsesToDepress; i++)
		{
			syn = &synapse[synapsesToDepress[i]];
			int toneuron = abs(syn->toneuron);
			stdp = neuron[toneuron].STDP;
	
			//check to see if a given synapses to neuron has just fired
			//otherwise we have to punish the incoming synapse. 
			if (neuron[toneuron].v < 30.)  //this should never happen since we swap numSynapsesToDepress with startNumSynapses to depress;
				syn->delta -= stdp; //punish by the to neurons stdp timer		
		}
		
		saveneuronactivation = neuronactivation;
		neuronactivation = newneuronactivation;
		newneuronactivation = saveneuronactivation;
		//I feel this must be done here sorry no other exp  
		//I did have it outside the brainsteps........how stupid!
		for (i = 0; i < dims->numNeurons; i++)	
		{
			if(neuron[i].v>30)
				neuron[i].STDP = STDP_RESET;
			else		
				neuron[i].STDP *= STDP_DEGRADATION_SCALER;	
		}

	}//end brainsteps
	
	 	
	//now this is where learning actually takes place.  It's here that we take the delta's we've been modifying
	//this whole time and actually use them to modify the efficacy of the synapses.  The reason we wait to modify
	//the actual synapse efficacies until all brain steps is complete is two fold one Izhikevich does it and two
	//for the sake of efficiency.
	float learningrate;
	// float half_max_weight = .5f * Brain::config.maxWeight, one_minus_decay = 1. - Brain::config.decayRate;
				
    for (k = 0; k < dims->numSynapses; k++)
    {
		learningrate = synapse[k].lrate;
		synapse[k].delta *= .9; //cheating a little
		if (synapse[k].efficacy >= 0)
			synapse[k].efficacy += (.01 + synapse[k].delta * learningrate);// * delta t; need a delta t 
		else
			synapse[k].efficacy -= (.01 + synapse[k].delta * learningrate);

        if (fabs(synapse[k].efficacy) > (0.5f * Brain::config.maxWeight))
        {
            synapse[k].efficacy *= 1.0f - (1.0f - Brain::config.decayRate) *
                (fabs(synapse[k].efficacy) - 0.5f * Brain::config.maxWeight) / (0.5f * Brain::config.maxWeight);
            if (synapse[k].efficacy > Brain::config.maxWeight)
                synapse[k].efficacy = Brain::config.maxWeight;
            else if (synapse[k].efficacy < -Brain::config.maxWeight)
                synapse[k].efficacy = -Brain::config.maxWeight;
        }
        else
        {
            if (learningrate >= 0.0f)  // excitatory
                synapse[k].efficacy = max(0.0f, synapse[k].efficacy);
            if (learningrate < 0.0f)  // inhibitory
                synapse[k].efficacy = min(-1.e-10f, synapse[k].efficacy);
        }
    }

/*
        learningrate = grouplrate[index4(neuron[i].group,neuron[j].group,ii,jj, dims->numgroups,2,2)];
		synapse[k].delta *= .9; //cheating a little iz did it
		if (synapse[k].efficacy >= 0)
		{
			synapse[k].efficacy += .01 + synapse[k].delta * learningrate;// * delta t; need a delta t 
			if (synapse[k].efficacy > Brain::config.maxWeight)
                synapse[k].efficacy = Brain::config.maxWeight - (one_minus_decay);
			else if (synapse[k].efficacy > half_max_weight)
				synapse[k].efficacy *= 1.0f - (one_minus_decay) * (synapse[k].efficacy - half_max_weight) / (half_max_weight);
			else
				synapse[k].efficacy = max(0.0f, synapse[k].efficacy);
		}
		else 
		{
			synapse[k].efficacy -= .01 + synapse[k].delta * learningrate;
			if (synapse[k].efficacy < -Brain::config.maxWeight)
                synapse[k].efficacy = -Brain::config.maxWeight + (one_minus_decay);
			else if(synapse[k].efficacy < -half_max_weight)
			//fix me later should be a      +
				synapse[k].efficacy *= 1.0f - one_minus_decay * (fabs(synapse[k].efficacy) - half_max_weight) / half_max_weight;
			else
				synapse[k].efficacy = min(-1.e-10f, synapse[k].efficacy);
		
		}
	}
*/
	

	//simulation.cpp readworldfile
	//when you change bump the version number
	//when version < new version then set spiking to false.
	//tsimulation object might want fUseSpikingModel or something similair.	

	// compute smoothed output unit activation levels
	
	float currentActivationLevel[dims->numOutputNeurons], scale_total_spikes = 1.0-scale_latest_spikes;
	for (i = 0; i < dims->numOutputNeurons; i++)
	{
		neuron[i+dims->getFirstOutputNeuron()].maxfiringcount = max(outputNeuronFiringCounter[i], (int) neuron[i+dims->getFirstOutputNeuron()].maxfiringcount);

#if USE_BIAS		
		currentActivationLevel[i]=fmin(1.0, (double)outputNeuronFiringCounter[i] / (double)BrainStepsPerWorldStep);
#else
		currentActivationLevel[i]=fmin(1.0, (double)outputNeuronFiringCounter[i] / (double)neuron[i+dims->getFirstOutputNeuron()].maxfiringcount);
#endif
		outputActivation[i] = scale_total_spikes * outputActivation[i]  +  scale_latest_spikes * currentActivationLevel[i];

		neuronactivation[i+dims->getFirstOutputNeuron()] = outputActivation[i];

	}


//###################################################################################################################################	
	
	
//watch your ass buddy I got a damn sigbus here check out run_sigbus do I need to lock the file
//this can't be threaded.  I did switch moniters maybe that cause the error?????
#if 1
	if(fHandle)
	{
		for(int i = 0; i < dims->getFirstOutputNeuron(); i++ )
		{
			fprintf( fHandle, "%d %1.4f\t", i, (float)NeuronFiringCounter[i]/BrainStepsPerWorldStep);
			for(int j=0; j<BrainStepsPerWorldStep; j++)
				fprintf( fHandle, "%c", spikeMatrix[i][j] );
			fprintf( fHandle, "\n");
		}
		for (int i = dims->getFirstOutputNeuron(); i < dims->numNeurons; i++)
		{
			fprintf( fHandle, "%d %1.4f\t", i, neuronactivation[i]);
			for(int j=0; j<BrainStepsPerWorldStep; j++)
				fprintf( fHandle, "%c", spikeMatrix[i][j] );
			fprintf( fHandle, "\n");
		}

	}
#endif
}
