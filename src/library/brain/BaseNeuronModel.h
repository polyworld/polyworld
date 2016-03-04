#pragma once

#include <assert.h>
#include <gl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "Brain.h"
#include "NervousSystem.h"
#include "NeuronModel.h"
#include "sim/globals.h"
#include "utils/AbstractFile.h"
#include "utils/misc.h"

template <typename T_neuron, typename T_neuronattrs, typename T_synapse>
class BaseNeuronModel : public NeuronModel
{
 public:
	BaseNeuronModel( NervousSystem *cns )
	{
		this->cns = cns;

		neuron = NULL;
		neuronactivation = NULL;
		newneuronactivation = NULL;
		synapse = NULL;

#if PrintBrain
		bprinted = false;
#endif
	}

	virtual ~BaseNeuronModel()
	{
		free( neuron );
		free( neuronactivation );
		free( newneuronactivation );
		free( synapse );
	}

	virtual void init_derived( double initial_activation ) = 0;

	virtual void init( Dimensions *dims,
					   double initial_activation )
	{
		this->dims = dims;

#define __ALLOC(NAME, TYPE, N) if(NAME) free(NAME); NAME = (TYPE *)calloc(N, sizeof(TYPE)); assert(NAME);

		__ALLOC( neuron, T_neuron, dims->numNeurons );
		__ALLOC( neuronactivation, double, dims->numNeurons );
		__ALLOC( newneuronactivation, double, dims->numNeurons );

		__ALLOC( synapse, T_synapse, dims->numSynapses );

#undef __ALLOC

		citfor( NervousSystem::NerveList, cns->getNerves(), it )
		{
			Nerve *nerve = *it;

			nerve->config( &(this->neuronactivation), &(this->newneuronactivation) );
		}		

		init_derived( initial_activation );
	}

	virtual void set_neuron( int index,
							 void *attributes,
							 int startsynapses,
							 int endsynapses )
	{
		T_neuron &n = neuron[index];
		T_neuronattrs *attrs = (T_neuronattrs *)attributes;

		assert( !isnan(attrs->bias) );

		n.bias = attrs->bias;
		n.startsynapses = startsynapses;
		n.endsynapses = endsynapses;

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
            cout << "    bias = " << n.bias nlf;
            cout << "    startsynapses = " << n.startsynapses nlf;
		}
#endif
	}

	virtual void set_neuron_endsynapses( int index,
										 int endsynapses )
	{
		T_neuron &n = neuron[index];

		n.endsynapses = endsynapses;
	}

	virtual void set_synapse( int index,
							  int from,
							  int to,
							  float efficacy,
							  float lrate )
	{
		T_synapse &s = synapse[index];

		assert( !isnan(efficacy) );
		assert( !isnan(lrate) );

		s.fromneuron = from;
		s.toneuron = to;
		s.efficacy = efficacy;
		s.lrate = lrate;

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "        synapse[" << index
				 << "].toneur, fromneur, efficacy = "
				 << s.toneuron cms s.fromneuron cms s.efficacy nlf;
		}
#endif

	}

	virtual void dumpAnatomical( AbstractFile *file )
	{
		size_t	dimCM;
		float*	connectionMatrix;
		short	i,j;
		long	s;
		float	maxWeight = max( Brain::config.maxWeight, Brain::config.maxbias );
		double	inverseMaxWeight = 1. / maxWeight;
		long imin = 10000;
		long imax = -10000;

		dimCM = (dims->numNeurons+1) * (dims->numNeurons+1);	// +1 for bias neuron
		connectionMatrix = (float*) calloc( sizeof( *connectionMatrix ), dimCM );
		if( !connectionMatrix )
		{
			fprintf( stderr, "%s: unable to alloca connectionMatrix\n", __FUNCTION__ );
			return;
		}

		daPrint( "%s: before filling connectionMatrix\n", __FUNCTION__ );

		// compute the connection matrix
		// columns correspond to presynaptic "from-neurons"
		// rows correspond to postsynaptic "to-neurons"
		for( s = 0; s < dims->numSynapses; s++ )
		{
			int cmIndex;
			cmIndex = abs(synapse[s].fromneuron)  +  abs(synapse[s].toneuron) * (dims->numNeurons + 1);	// +1 for bias neuron
			if( cmIndex < 0 )
			{
				fprintf( stderr, "cmIndex = %d, s = %ld, fromneuron = %d, toneuron = %d, numneurons = %d\n", cmIndex, s, synapse[s].fromneuron, synapse[s].toneuron, dims->numNeurons );
			}
			daPrint( "  s=%d, fromneuron=%d, toneuron=%d, cmIndex=%d\n", s, synapse[s].fromneuron, synapse[s].toneuron, cmIndex );
			connectionMatrix[cmIndex] += synapse[s].efficacy;	// the += is so parallel excitatory and inhibitory connections from input and output neurons just sum together
			if( cmIndex < imin )
				imin = cmIndex;
			if( cmIndex > imax )
				imax = cmIndex;
		}
	
		// fill in the biases
		for( i = 0; i < dims->numNeurons; i++ )
		{
			int cmIndex = dims->numNeurons  +  i * (dims->numNeurons + 1);
			connectionMatrix[cmIndex] = neuron[i].bias;
			if( cmIndex < imin )
				imin = cmIndex;
			if( cmIndex > imax )
				imax = cmIndex;
		}

		if( imin < 0 )
			fprintf( stderr, "%s: cmIndex < 0 (%ld)\n", __FUNCTION__, imin );
		if( imax > (dims->numNeurons+1)*(dims->numNeurons+1) )
			fprintf( stderr, "%s: cmIndex > (numneurons+1)^2 (%ld > %d)\n", __FUNCTION__, imax, (dims->numNeurons+1)*(dims->numNeurons+1) );

		daPrint( "%s: imin = %ld, imax = %ld, numneurons = %d (+1 for bias)\n", __FUNCTION__, imin, imax, dims->numNeurons );

		daPrint( "%s: file = %08lx, index = %ld, fitness = %g\n", __FUNCTION__, (char*)file, index, fitness );

		// print the network architecture
		for( i = 0; i <= dims->numNeurons; i++ )	// running over post-synaptic neurons + bias ('=' because of bias)
		{
			for( j = 0; j <= dims->numNeurons; j++ )	// running over pre-synaptic neurons + bias ('=' because of bias)
				file->printf( "%+06.4f ", connectionMatrix[j + i*(dims->numNeurons+1)] * inverseMaxWeight );
			file->printf( ";\n" );
		}
		
		free( connectionMatrix );
	}

	virtual void startFunctional( AbstractFile *file )
	{
		file->printf( " %d %d %d %ld",
					  dims->numNeurons, dims->numInputNeurons, dims->numOutputNeurons, dims->numSynapses );		
	}

	virtual void writeFunctional( AbstractFile *file )
	{
		for( int i = 0; i < dims->numNeurons; i++ )
		{
			file->printf( "%d %g\n", i, neuronactivation[i] );
		}
	}

	//protected:
	NervousSystem *cns;
	Dimensions *dims;

	T_neuron *neuron;
	double *neuronactivation;
	double *newneuronactivation;
	T_synapse *synapse;

#if PrintBrain
	bool bprinted;
#endif
};
