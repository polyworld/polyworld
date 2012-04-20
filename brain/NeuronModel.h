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
		int numneurons;
		long numsynapses;
		
		int numInputNeurons;
		int numNonInputNeurons;
		int numOutputNeurons;

		int firstOutputNeuron;
		int firstNonInputNeuron;
		int firstInternalNeuron;
	};

	virtual ~NeuronModel() {}

	virtual void init( Dimensions *dims,
					   float initial_activation ) = 0;

	virtual void set_neuron( int index,
							 void *attributes,
							 int startsynapses = -1,
							 int endsynapses = -1 ) = 0;
	virtual void set_neuron_endsynapses( int index,
										 int endsynapses ) = 0;
	virtual void set_synapse( int index,
							  int from,
							  int to,
							  float efficacy,
							  float lrate ) = 0;

	virtual void update( bool bprint ) = 0;

	virtual void dumpAnatomical( AbstractFile *file ) = 0;

	virtual void startFunctional( AbstractFile *file ) = 0;
	virtual void writeFunctional( AbstractFile *file ) = 0;
};
