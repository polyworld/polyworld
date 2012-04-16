#pragma once

#include <assert.h>
#include <gl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "AbstractFile.h"
#include "Brain.h"
#include "Genome.h"
#include "globals.h"
#include "misc.h"
#include "NervousSystem.h"
#include "NeuronModel.h"

template <typename T_neuron, typename T_synapse>
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
		grouplrate = NULL;

#if PrintBrain
		bprinted = false;
#endif
		
#if DesignerBrains
		groupsize = NULL;
#endif
	}

	virtual ~BaseNeuronModel()
	{
		free( neuron );
		free( neuronactivation );
		free( newneuronactivation );
		free( synapse );
		free( grouplrate );

#if DesignerBrains
		free( groupsize );
#endif
	}

	virtual void init_derived( float initial_activation ) = 0;

	virtual void init( genome::Genome *genes,
					   Dimensions *dims,
					   float initial_activation )
	{
		this->genes = genes;
		this->dims = dims;

#define __ALLOC(NAME, TYPE, N) if(NAME) free(NAME); NAME = (TYPE *)calloc(N, sizeof(TYPE)); assert(NAME);

		__ALLOC( neuron, T_neuron, dims->numneurons );
		__ALLOC( neuronactivation, float, dims->numneurons );
		__ALLOC( newneuronactivation, float, dims->numneurons );

		__ALLOC( synapse, T_synapse, dims->numsynapses );

		__ALLOC( grouplrate, float, dims->numgroups * dims->numgroups * 4 );

#if DesignerBrains
		__ALLOC( groupsize, int, dims->numgroups );
#endif

#undef __ALLOC

#if DesignerBrains
		memset( groupsize, 0, sizeof(int) * dims->numgroups );
#endif

		for( NervousSystem::nerve_iterator
				 it = cns->begin_nerve(),
				 end = cns->end_nerve();
			 it != end;
			 it++ )
		{
			Nerve *nerve = *it;

			nerve->config( &(this->neuronactivation), &(this->newneuronactivation) );
		}		

		init_derived( initial_activation );
	}

	virtual void set_neuron( int index,
							 int group,
							 float bias,
							 int startsynapses,
							 int endsynapses )
	{
		T_neuron &n = neuron[index];

		n.group = group;
		n.bias = bias;
		n.startsynapses = startsynapses;
		n.endsynapses = endsynapses;

#if DesignerBrains
		groupsize[group]++;
#endif

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
            cout << "    group = " << n.group nlf;
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
							  float efficacy )
	{
		T_synapse &s = synapse[index];

		s.fromneuron = from;
		s.toneuron = to;
		s.efficacy = efficacy;

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "        synapse[" << index
				 << "].toneur, fromneur, efficacy = "
				 << s.toneuron cms s.fromneuron cms s.efficacy nlf;
		}
#endif

	}


	virtual void set_grouplrate( genome::SynapseType *syntype,
								 int from,
								 int to,
								 float lrate )
	{
		int ifrom = syntype->nt_from == genome::EXCITATORY ? 0 : 1;
		int ito = syntype->nt_to == genome::EXCITATORY ? 0 : 1;

		grouplrate[index4(to, from, ito, ifrom,
						  dims->numgroups, 2, 2)] = lrate;
	}


	virtual void dumpAnatomical( AbstractFile *file )
	{
		size_t	dimCM;
		float*	connectionMatrix;
		short	i,j;
		long	s;
		float	maxWeight = max( brain::gMaxWeight, brain::gNeuralValues.maxbias );
		double	inverseMaxWeight = 1. / maxWeight;
		long imin = 10000;
		long imax = -10000;

		dimCM = (dims->numneurons+1) * (dims->numneurons+1);	// +1 for bias neuron
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
		for( s = 0; s < dims->numsynapses; s++ )
		{
			int cmIndex;
			cmIndex = abs(synapse[s].fromneuron)  +  abs(synapse[s].toneuron) * (dims->numneurons + 1);	// +1 for bias neuron
			if( cmIndex < 0 )
			{
				fprintf( stderr, "cmIndex = %d, s = %ld, fromneuron = %d, toneuron = %d, numneurons = %d\n", cmIndex, s, synapse[s].fromneuron, synapse[s].toneuron, dims->numneurons );
			}
			daPrint( "  s=%d, fromneuron=%d, toneuron=%d, cmIndex=%d\n", s, synapse[s].fromneuron, synapse[s].toneuron, cmIndex );
			connectionMatrix[cmIndex] += synapse[s].efficacy;	// the += is so parallel excitatory and inhibitory connections from input and output neurons just sum together
			if( cmIndex < imin )
				imin = cmIndex;
			if( cmIndex > imax )
				imax = cmIndex;
		}
	
		// fill in the biases
		for( i = 0; i < dims->numneurons; i++ )
		{
			int cmIndex = dims->numneurons  +  i * (dims->numneurons + 1);
			connectionMatrix[cmIndex] = neuron[i].bias;
			if( cmIndex < imin )
				imin = cmIndex;
			if( cmIndex > imax )
				imax = cmIndex;
		}

		if( imin < 0 )
			fprintf( stderr, "%s: cmIndex < 0 (%ld)\n", __FUNCTION__, imin );
		if( imax > (dims->numneurons+1)*(dims->numneurons+1) )
			fprintf( stderr, "%s: cmIndex > (numneurons+1)^2 (%ld > %d)\n", __FUNCTION__, imax, (dims->numneurons+1)*(dims->numneurons+1) );

		daPrint( "%s: imin = %ld, imax = %ld, numneurons = %d (+1 for bias)\n", __FUNCTION__, imin, imax, dims->numneurons );

		daPrint( "%s: file = %08lx, index = %ld, fitness = %g\n", __FUNCTION__, (char*)file, index, fitness );

		// print the network architecture
		for( i = 0; i <= dims->numneurons; i++ )	// running over post-synaptic neurons + bias ('=' because of bias)
		{
			for( j = 0; j <= dims->numneurons; j++ )	// running over pre-synaptic neurons + bias ('=' because of bias)
				file->printf( "%+06.4f ", connectionMatrix[j + i*(dims->numneurons+1)] * inverseMaxWeight );
			file->printf( ";\n" );
		}
		
		free( connectionMatrix );
	}

	virtual void startFunctional( AbstractFile *file )
	{
		file->printf( " %d %d %d %ld",
					  dims->numneurons, dims->numInputNeurons, dims->numOutputNeurons, dims->numsynapses );		
	}

	virtual void writeFunctional( AbstractFile *file )
	{
		for( int i = 0; i < dims->numneurons; i++ )
		{
			file->printf( "%d %g\n", i, neuronactivation[i] );
		}
	}

	virtual void render( short patchwidth, short patchheight )
	{
		if ((neuron == NULL) || (synapse == NULL))
			return;

		short i;
		short x1, x2;
		short xoff,yoff;
		long k;

		// the neuronactivation and newneuronactivation arrays have been
		// repointered by this time, so their contents are the reverse of
		// their names in this routine

		// this horizontal row of elements shows the new inputs and the states
		// at the previous time step used to calculate the new values
		short y1 = patchheight;
		short y2 = y1 + patchheight;
		for (i = 0, x1 = 2 * patchwidth; i < short(dims->numneurons); i++, x1+=patchwidth)
		{
			// the following reference to "newneuron" really gets the old
			// values, except for the clamped input neuron values (which are new)
			const unsigned char mag = (unsigned char)(newneuronactivation[i] * 255.);
			glColor3ub(mag, mag, mag);
			glRecti(x1, y1, x1 + patchwidth, y2);
		}

		// this vertical row of elements shows the biases (unfortunately the
		// new, updated values rather than those actually used to compute the
		// neuronal states which are adjacent) in all non-input neurons
		x1 = 0;
		x2 = patchwidth;
		for (i = dims->firstNonInputNeuron, y1 = 2*patchheight; i < dims->numneurons; i++, y1 += patchheight)
		{
			const unsigned char mag = (unsigned char)((brain::gMaxWeight + neuron[i].bias) * 127.5 / brain::gMaxWeight);
			glColor3ub(mag, mag, mag);
			glRecti(x1, y1, x2, y1 + patchheight);
		}

		// this vertical row of elements shows the new states in all the
		// unclamped neurons, including the output neurons
		x1 = x2;
		x2 = x1 + patchwidth;
		for (i = dims->firstNonInputNeuron, y1 = 2*patchheight; i < dims->numneurons;
			 i++, y1 += patchheight)
		{
			const unsigned char mag = (unsigned char)(neuronactivation[i] * 255.);
			glColor3ub(mag, mag, mag);
			glRecti(x1, y1, x2, y1 + patchheight);
		}

		// this array of synaptic strengths unfortunately shows the new, updated
		// values rather than those actually used to map the displayed horizontal
		// row of neuronal values onto the displayed vertical row of values
		xoff = 2 * patchwidth;
		yoff = 2 * patchheight;

		// draw the neutral gray background
		glColor3ub(127, 127, 127);
		glRecti(xoff,
				yoff,
				xoff + short(dims->numneurons) * patchwidth,
				yoff + short(dims->numNonInputNeurons) * patchheight);
	
		glLineWidth( 1.0 );	
		rPrint( "**************************************************************\n");
		for (k = 0; k < dims->numsynapses; k++)
		{
			const unsigned char mag = (unsigned char)((brain::gMaxWeight + synapse[k].efficacy) * 127.5 / brain::gMaxWeight);

			// Fill the rect
			glColor3ub(mag, mag, mag);
			x1 = xoff  +   abs(synapse[k].fromneuron) * patchwidth;
			y1 = yoff  +  (abs(synapse[k].toneuron)-dims->firstNonInputNeuron) * patchheight;   

			if( abs( synapse[k].fromneuron ) < dims->firstInternalNeuron )	// input or output neuron, so it can be both excitatory and inhibitory
			{
				if( synapse[k].efficacy >= 0.0 )	// excitatory
				{
					// fill it
					glRecti( x1, y1 + patchheight/2, x1 + patchwidth, y1 + patchheight );
					rPrint( "+" );

					// frame it
					glColor3ub( 255, 255, 255 );
					glBegin( GL_LINE_LOOP );
					glVertex2i( x1, y1 + patchheight/2 );
					glVertex2i( x1 + patchwidth, y1 + patchheight/2);
					glVertex2i( x1 + patchwidth, y1 + patchheight );
					glVertex2i( x1, y1 + patchheight);        	
					glEnd();       
				}
				else	// inhibitory
				{
					// fill it
					glRecti( x1, y1, x1 + patchwidth, y1 + patchheight/2 );
					rPrint( "-" );

					// frame it
					glColor3ub( 0, 0, 0 );
					glLineWidth( 1.0 );
					glBegin( GL_LINE_LOOP );
					glVertex2i( x1, y1 );
					glVertex2i( x1 + patchwidth, y1 );
					glVertex2i( x1 + patchwidth, y1 + patchheight/2 );
					glVertex2i( x1, y1 + patchheight/2 );
					glEnd();       
				}
			}
			else	// all other neurons and synapses
			{
				// fill it
				glRecti( x1, y1, x1 + patchwidth, y1 + patchheight );
				rPrint( " " );

				// frame it
				if( synapse[k].efficacy >= 0.0 )	// excitatory
					glColor3ub( 255, 255, 255 );
				else	// inhibitory
					glColor3ub( 0, 0, 0 );
				glLineWidth( 1.0 );
				glBegin(GL_LINE_LOOP );
				glVertex2i( x1, y1 );
				glVertex2i( x1 + patchwidth, y1 );
				glVertex2i( x1 + patchwidth, y1 + patchheight );
				glVertex2i( x1, y1 + patchheight );        	
				glEnd();       
			}
			rPrint( "k = %ld, eff = %5.2f, mag = %d, x1 = %d, y1 = %d, abs(from) = %d, abs(to) = %d, firstOutputNeuron = %d, firstInternalNeuron = %d\n",
					k, synapse[k].efficacy, mag, x1, y1, abs(synapse[k].fromneuron), abs(synapse[k].toneuron), dims->firstOutputNeuron, dims->firstInternalNeuron );
		}

		//
		// Now highlight the neuronal groups for clarity
		//
		Nerve *nerve;

		// Red
		nerve = cns->get("red");
		if( nerve )
		{
			y1 = patchheight;
			y2 = y1 + patchheight;

			x1 = short(nerve->getIndex())*patchwidth + xoff;
			x2 = x1 + nerve->getNeuronCount()*patchwidth;

			glColor3ub(255, 0, 0);
			glLineWidth(1.0);	
			glBegin(GL_LINE_LOOP);
			glVertex2i(x1, y1);
			glVertex2i(x2 - 1, y1);
			glVertex2i(x2 - 1, y2 + 1);
			glVertex2i(x1, y2);        	
			glEnd();        
		}

		// Green
		nerve = cns->get("green");
		if( nerve )
		{
			x1 = x2;
			x2 = x1 + nerve->getNeuronCount() * patchwidth;
			glColor3ub(0, 255, 0);
			glLineWidth(1.0);	
			glBegin(GL_LINE_LOOP);
			glVertex2i(x1, y1);
			glVertex2i(x2 - 1, y1);
			glVertex2i(x2 - 1, y2 + 1);
			glVertex2i(x1, y2);        	
			glEnd();
		}

		// Blue
		nerve = cns->get("blue");
		if( nerve )
		{
			x1 = x2;
			x2 = x1 + nerve->getNeuronCount() * patchwidth;
			glColor3ub(0, 0, 255);
			glLineWidth(1.0);
			glBegin(GL_LINE_LOOP);
			glVertex2i(x1, y1);
			glVertex2i(x2, y1);
			glVertex2i(x2, y2 + 1);
			glVertex2i(x1, y2);        	
			glEnd();        
		}

		// Frame the groups
		glColor3ub(255, 255, 255);
		glLineWidth(1.0);	
		x2 = dims->numInputNeurons * patchwidth + xoff;
		for (i = brain::gNeuralValues.numinputneurgroups; i < dims->numgroups; i++)
		{
			short numneur;
	
#if DesignerBrains
			// For DesignerBrains, the genes are meaningless
			numneur = groupsize[i];
#else
			numneur = genes->getNeuronCount(i);
#endif
	
			x1 = x2;
			x2 = x1 + numneur * patchwidth;
        
			glBegin(GL_LINE_LOOP);
        	glVertex2i(x1, y1);
        	glVertex2i(x2, y1);
        	glVertex2i(x2, y2 /*+ 1*/);
        	glVertex2i(x1, y2);        	
			glEnd();
		}

		x1 = patchwidth;
		x2 = x1 + patchwidth;
		y2 = yoff;
    
		for (i = brain::gNeuralValues.numinputneurgroups; i < dims->numgroups; i++)
		{
			short numneur;
		
#if DesignerBrains
			// For DesignerBrains, the genes are meaningless
			numneur = groupsize[i];
#else
			numneur = genes->getNeuronCount(i);
#endif
	
			y1 = y2;
			y2 = y1 + numneur * patchheight;
        
			glBegin(GL_LINE_LOOP);
        	glVertex2i(x1, y1);
        	glVertex2i(x2, y1);
        	glVertex2i(x2, y2 /*+ 1*/);
        	glVertex2i(x1, y2);        	
			glEnd();
		}

	}

 protected:
 public: // temporary public for testing
	NervousSystem *cns;
	genome::Genome *genes;

	Dimensions *dims;

	T_neuron *neuron;
	float *neuronactivation;
	float *newneuronactivation;
	T_synapse *synapse;
	float *grouplrate;

#if PrintBrain
	bool bprinted;
#endif

#if DesignerBrains
	int *groupsize;
#endif
};
