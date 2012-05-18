#pragma once

#include "BaseNeuronModel.h"

// forward decls
class NervousSystem;

// note: activation levels are not maintained in the neuronstruct
// so that after the new activation levels are computed, the old
// and new blocks of memory can simply be repointered rather than
// copied.
struct FiringRateModel__Neuron
{
	float bias;
	float tau;
	long  startsynapses;
	long  endsynapses;
};

struct FiringRateModel__NeuronAttrs
{
	float bias;
	float tau;
};

struct FiringRateModel__Synapse
{
	float efficacy;   // > 0 for excitatory, < 0 for inhibitory
	float lrate;
	short fromneuron;
	short toneuron;
};


class FiringRateModel : public BaseNeuronModel<FiringRateModel__Neuron, FiringRateModel__NeuronAttrs, FiringRateModel__Synapse>
{
	typedef FiringRateModel__Neuron Neuron;
	typedef FiringRateModel__NeuronAttrs NeuronAttrs;
	typedef FiringRateModel__Synapse Synapse;

 public:
	FiringRateModel( NervousSystem *cns );
	virtual ~FiringRateModel();

	virtual void init_derived( float initial_activation );

	virtual void set_neuron( int index,
							 void *attributes,
							 int startsynapses,
							 int endsynapses );

	virtual void update( bool bprint );
};
