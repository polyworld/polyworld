#pragma once

#include "BaseNeuronModel.h"

// forward decls
class NervousSystem;
namespace genome
{
	class Gene;
};

// note: activation levels are not maintained in the neuronstruct
// so that after the new activation levels are computed, the old
// and new blocks of memory can simply be repointered rather than
// copied.
struct FiringRateModel__Neuron
{
	short group;
	float bias;
	float tau;
	long  startsynapses;
	long  endsynapses;
};

struct FiringRateModel__Synapse
{
	float efficacy;   // > 0 for excitatory, < 0 for inhibitory
	short fromneuron; // > 0 for excitatory, < 0 for inhibitory
	short toneuron;   // > 0 for excitatory, < 0 for inhibitory
};


class FiringRateModel : public BaseNeuronModel<FiringRateModel__Neuron, FiringRateModel__Synapse>
{
	typedef FiringRateModel__Neuron Neuron;
	typedef FiringRateModel__Synapse Synapse;

 public:
	FiringRateModel( NervousSystem *cns );
	virtual ~FiringRateModel();

	virtual void init_derived( float initial_activation );

	virtual void set_neuron( int index,
							 int group,
							 float bias,
							 int startsynapses,
							 int endsynapses );

	virtual void inheritState( NeuronModel *parent );

	virtual void dump( std::ostream &out );
	virtual void load( std::istream &in );

	virtual void update( bool bprint );

 private:
	genome::Gene *tauGene;
};
