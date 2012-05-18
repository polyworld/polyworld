#pragma once

#include "BaseNeuronModel.h"

#define USE_BIAS				true
#define SpikingActivation       25.0
#define BIAS_INJECTED_VOLTAGE   208.0
#define STDP_RESET				.1
#define STDP_DEGRADATION_SCALER .95
#define STDP_DEPRESSION_SCALER	.5
#define BrainStepsPerWorldStep  50
#define MaxFiringRatePerSecond  260.0
#define MinFiringRatePerSecond  0.0

// note: activation levels are not maintained in the neuronstruct
// so that after the new activation levels are computed, the old
// and new blocks of memory can simply be repointered rather than
// copied.
struct SpikingModel__Neuron
{
	short group;
	float bias;
	long  startsynapses;
	long  endsynapses;
	float v;              //!<represents the membrane potential of the neuron 
	float u;			  //!<the membranes recovery period			
	float STDP;           //!<spike-timing-dependent plasticity,
	short maxfiringcount; //explain later if works
	double SpikingParameter_a;
	double SpikingParameter_b;
	double SpikingParameter_c;
	double SpikingParameter_d;
};

struct SpikingModel__NeuronAttrs
{
	float bias;
	double SpikingParameter_a;
	double SpikingParameter_b;
	double SpikingParameter_c;
	double SpikingParameter_d;
};

struct SpikingModel__Synapse
{
	float efficacy;   // > 0 for excitatory, < 0 for inhibitory
	float lrate;
	short fromneuron;
	short toneuron;
	float delta;  //!from iz intead of effecting weights directly
};

// forward decls
class NervousSystem;
class RandomNumberGenerator;

class SpikingModel : public BaseNeuronModel<SpikingModel__Neuron, SpikingModel__NeuronAttrs, SpikingModel__Synapse>
{
	typedef SpikingModel__Neuron Neuron;
	typedef SpikingModel__NeuronAttrs NeuronAttrs;
	typedef SpikingModel__Synapse Synapse;

 public:
	SpikingModel( NervousSystem *cns, float scale_latest_spikes );
	virtual ~SpikingModel();

	virtual void init_derived( float initial_activation );

	virtual void set_neuron( int index,
							 void *attributes,
							 int startsynapses,
							 int endsynapses );

	virtual void update( bool bprint );

 private:
	RandomNumberGenerator *rng;

	float scale_latest_spikes;

	float *outputActivation;
};
