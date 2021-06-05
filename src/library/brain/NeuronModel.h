#pragma once

#include <stdio.h>

#include <iostream>

// forward decls
class AbstractFile;

#define DebugDumpAnatomical false
#if DebugDumpAnatomical
	#define daPrint( x... ) printf( x );
#else
	#define daPrint( x... )
#endif


class NeuronModel
{
 public:
	struct Dimensions
	{
		Dimensions() { numNeurons = numInputNeurons = numOutputNeurons = numSynapses = 0; }

		int numNeurons;
		int numInputNeurons;
		int numOutputNeurons;

		long numSynapses;

		inline int getFirstInputNeuron() { return 0; }
		inline int getFirstOutputNeuron() { return numInputNeurons; }
		inline int getFirstInternalNeuron() { return numInputNeurons + numOutputNeurons; }

		inline int getNumNonInputNeurons() { return numNeurons - numInputNeurons; }
	};

	virtual ~NeuronModel() {}

	virtual void init( Dimensions *dims,
					   double initial_activation ) = 0;

	virtual void set_neuron( int index,
							 void *attributes,
							 int startsynapses = -1,
							 int endsynapses = -1 ) = 0;
	virtual void set_neuron_endsynapses( int index,
										 int endsynapses ) = 0;
	virtual void get_synapse( int index,
							  short &from,
							  short &to,
							  float &efficacy,
							  float &lrate ) = 0;
	virtual void set_synapse( int index,
							  int from,
							  int to,
							  float efficacy,
							  float lrate ) = 0;

	virtual void update( bool bprint ) = 0;

	virtual void getActivations( double *activations, int start, int count ) = 0;
	virtual void setActivations( double *activations, int start, int count ) = 0;
	virtual void randomizeActivations() = 0;

	virtual void dumpAnatomical( AbstractFile *file ) = 0;

	virtual void startFunctional( AbstractFile *file ) = 0;
	virtual void writeFunctional( AbstractFile *file ) = 0;

	virtual void dumpSynapses( AbstractFile *file ) = 0;
	virtual void loadSynapses( AbstractFile *file ) = 0;
	virtual void copySynapses( NeuronModel *other ) = 0;
	virtual void scaleSynapses( float factor ) = 0;
};
