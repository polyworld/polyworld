#pragma once

#include "GroupsGenome.h"
#include "NeuralNetRenderer.h"

template <typename T_neuronModel>
class GroupsNeuralNetRenderer : public NeuralNetRenderer
{
 public:
	GroupsNeuralNetRenderer( T_neuronModel *neuronModel, genome::GroupsGenome *genome )
		: _neuronModel( neuronModel )
		, _genome( genome )
	{
	}

	void getSize( short patchWidth, short patchHeight,
				  short *ret_width, short *ret_height )
	{
		*ret_width = _neuronModel->dims->numNeurons * patchWidth + 2 * patchWidth;						  
		*ret_height = _neuronModel->dims->getNumNonInputNeurons() * patchHeight + 2 * patchHeight;
	}

	void render( short patchwidth, short patchheight )
	{
		if ((_neuronModel->neuron == NULL) || (_neuronModel->synapse == NULL))
			return;

		int numgroups = _genome->getGroupCount(genome::NGT_ANY);

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
		for (i = 0, x1 = 2 * patchwidth; i < short(_neuronModel->dims->numNeurons); i++, x1+=patchwidth)
		{
			// the following reference to "newneuron" really gets the old
			// values, except for the clamped input neuron values (which are new)
			const unsigned char mag = (unsigned char)(_neuronModel->newneuronactivation[i] * 255.);
			glColor3ub(mag, mag, mag);
			glRecti(x1, y1, x1 + patchwidth, y2);
		}

		// this vertical row of elements shows the biases (unfortunately the
		// new, updated values rather than those actually used to compute the
		// neuronal states which are adjacent) in all non-input neurons
		x1 = 0;
		x2 = patchwidth;
		for (i = _neuronModel->dims->getFirstOutputNeuron(), y1 = 2*patchheight; i < _neuronModel->dims->numNeurons; i++, y1 += patchheight)
		{
			const unsigned char mag = (unsigned char)((Brain::config.maxWeight + _neuronModel->neuron[i].bias) * 127.5 / Brain::config.maxWeight);
			glColor3ub(mag, mag, mag);
			glRecti(x1, y1, x2, y1 + patchheight);
		}

		// this vertical row of elements shows the new states in all the
		// unclamped neurons, including the output neurons
		x1 = x2;
		x2 = x1 + patchwidth;
		for (i = _neuronModel->dims->getFirstOutputNeuron(), y1 = 2*patchheight; i < _neuronModel->dims->numNeurons;
			 i++, y1 += patchheight)
		{
			const unsigned char mag = (unsigned char)(_neuronModel->neuronactivation[i] * 255.);
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
				xoff + short(_neuronModel->dims->numNeurons) * patchwidth,
				yoff + short(_neuronModel->dims->getNumNonInputNeurons()) * patchheight);
	
		glLineWidth( 1.0 );	
		rPrint( "**************************************************************\n");
		for (k = 0; k < _neuronModel->dims->numSynapses; k++)
		{
			const unsigned char mag = (unsigned char)((Brain::config.maxWeight + _neuronModel->synapse[k].efficacy) * 127.5 / Brain::config.maxWeight);

			// Fill the rect
			glColor3ub(mag, mag, mag);
			x1 = xoff  +   abs(_neuronModel->synapse[k].fromneuron) * patchwidth;
			y1 = yoff  +  (abs(_neuronModel->synapse[k].toneuron)-_neuronModel->dims->getFirstOutputNeuron()) * patchheight;   

			if( abs( _neuronModel->synapse[k].fromneuron ) < _neuronModel->dims->getFirstInternalNeuron() )	// input or output neuron, so it can be both excitatory and inhibitory
			{
				if( _neuronModel->synapse[k].efficacy >= 0.0 )	// excitatory
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
				if( _neuronModel->synapse[k].efficacy >= 0.0 )	// excitatory
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
					k, _neuronModel->synapse[k].efficacy, mag, x1, y1, abs(_neuronModel->synapse[k].fromneuron), abs(_neuronModel->synapse[k].toneuron), _neuronModel->dims->getFirstOutputNeuron(), _neuronModel->dims->getFirstInternalNeuron() );
		}

		//
		// Now highlight the neuronal groups for clarity
		//
		Nerve *nerve;

		// Red
		nerve = _neuronModel->cns->getNerve( "Red" );
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
		nerve = _neuronModel->cns->getNerve( "Green" );
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
		nerve = _neuronModel->cns->getNerve( "Blue" );
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
		x2 = _neuronModel->dims->numInputNeurons * patchwidth + xoff;
		for (i = GroupsBrain::config.numinputneurgroups; i < numgroups; i++)
		{
			short numneur;
	
			numneur = _genome->getNeuronCount(i);
	
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
    
		for (i = GroupsBrain::config.numinputneurgroups; i < numgroups; i++)
		{
			short numneur;
		
			numneur = _genome->getNeuronCount(i);
	
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

 private:
	T_neuronModel *_neuronModel;
	genome::GroupsGenome *_genome;
};
