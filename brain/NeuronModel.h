#pragma once

#include <stdio.h>

#include <iostream>

#include "Genome.h"

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
		int numgroups;
		
		int numInputNeurons;
		int numNonInputNeurons;
		int numOutputNeurons;

		int firstOutputNeuron;
		int firstNonInputNeuron;
		int firstInternalNeuron;
	};

	virtual ~NeuronModel() {}

	virtual void init( genome::Genome *genes,
					   Dimensions *dims,
					   float initial_activation ) = 0;

	virtual void set_neuron( int index,
							 int group,
							 float bias,
							 int startsynapses = -1,
							 int endsynapses = -1 ) = 0;
	virtual void set_neuron_endsynapses( int index,
										 int endsynapses ) = 0;
	virtual void set_synapse( int index,
							  int from,
							  int to,
							  float efficacy ) = 0;

	virtual void set_grouplrate( genome::SynapseType *syntype,
								 int from,
								 int to,
								 float lrate ) = 0;

	virtual void set_groupblrate( int group,
								  float value ) = 0;

	virtual void inheritState( NeuronModel *parent ) = 0;

	virtual void update( bool bprint ) = 0;

	virtual void dumpAnatomical( AbstractFile *file ) = 0;

	virtual void startFunctional( AbstractFile *file ) = 0;
	virtual void writeFunctional( AbstractFile *file ) = 0;

	virtual void dump( std::ostream &out ) = 0;
	virtual void load( std::istream &in ) = 0;

	virtual void render( short patchwidth, short patchheight ) = 0;
};
