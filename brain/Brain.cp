/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// Self
#include "Brain.h"

#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

// qt
#include <qapplication.h>

// Local
#include "AbstractFile.h"
#include "agent.h"
#include "debug.h"
#include "FiringRateModel.h"
#include "GenomeUtil.h"
#include "globals.h"
#include "misc.h"
#include "NervousSystem.h"
#include "NeuronModel.h"
#include "RandomNumberGenerator.h"
#include "Simulation.h"
#include "SpikingModel.h"

using namespace genome;


// Internal globals
bool Brain::classinited;
#if DebugBrainGrow
	bool DebugBrainGrowPrint = true;
#endif

static float initminweight = 0.0; // could read this in

enum
{
	kSynapseTypeEE = 0,
	kSynapseTypeIE = 1,
	kSynapseTypeEI = 2,
	kSynapseTypeII = 3
};

#pragma mark -

#define __ALLOC_STACK_BUFFER(NAME, TYPE, N) TYPE *NAME = (TYPE *)alloca( N * sizeof(TYPE) ); Q_CHECK_PTR(NAME)
#define ALLOC_STACK_BUFFER(NAME, TYPE) __ALLOC_STACK_BUFFER( NAME, TYPE, __numgroups )

#define ALLOC_GROW_STACK_BUFFERS()										\
	int __numgroups = g->getGroupCount(NGT_ANY);			\
	ALLOC_STACK_BUFFER( firsteneur, short );							\
	ALLOC_STACK_BUFFER( firstineur, short );							\
	ALLOC_STACK_BUFFER( eeremainder, float );							\
	ALLOC_STACK_BUFFER( eiremainder, float );							\
	ALLOC_STACK_BUFFER( iiremainder, float );							\
	ALLOC_STACK_BUFFER( ieremainder, float );							\
	__ALLOC_STACK_BUFFER( neurused,										\
						  bool,											\
						  max((int)g->get("ExcitatoryNeuronCount_max"), \
							  (int)g->get("InhibitoryNeuronCount_max")) )

#define IsInputNeuralGroup( group ) ( mygenes->schema->getNeurGroupType(group) == NGT_INPUT )
#define IsOutputNeuralGroup( group ) ( mygenes->schema->getNeurGroupType(group) == NGT_OUTPUT )
#define IsInternalNeuralGroup( group ) ( mygenes->schema->getNeurGroupType(group) == NGT_INTERNAL )

//===========================================================================
// Brain
//===========================================================================

//---------------------------------------------------------------------------
// Brain::braininit
//---------------------------------------------------------------------------
void Brain::braininit()
{
    if (Brain::classinited)
        return;

    Brain::classinited = true;

 	// Set up retina values
    brain::retinawidth = max(brain::gMinWin, genome::gMaxvispixels);
    
    if (brain::retinawidth & 1)
        brain::retinawidth++;  // keep it even for efficiency (so I'm told)
        
    brain::retinaheight = brain::gMinWin;
    
    if (brain::retinaheight & 1)
        brain::retinaheight++;        
}


//---------------------------------------------------------------------------
// Brain::braindestruct
//---------------------------------------------------------------------------
void Brain::braindestruct()
{
}


//---------------------------------------------------------------------------
// Brain::Brain
//---------------------------------------------------------------------------
Brain::Brain(NervousSystem *_cns)
	:	cns(_cns),
		mygenes(NULL)	// but don't delete them, because we don't new them
{
	if (!Brain::classinited)
		braininit();	

	rng = cns->getRNG();

	switch( brain::gNeuralValues.model )
	{
	case brain::NeuralValues::SPIKING:
		neuralnet = new SpikingModel( cns );
		break;
	case brain::NeuralValues::FIRING_RATE:
	case brain::NeuralValues::TAU:
		neuralnet = new FiringRateModel( cns );
		break;
	default:
		assert(false);
	}
}


//---------------------------------------------------------------------------
// Brain::~Brain
//---------------------------------------------------------------------------
Brain::~Brain()
{
	delete neuralnet;
}

//---------------------------------------------------------------------------
// Brain::dumpAnatomical
//---------------------------------------------------------------------------
void Brain::dumpAnatomical( const char* directoryName, const char* suffix, long index, float fitness )
{
	AbstractFile *file;
	char	filename[256];

	sprintf( filename, "%s/brainAnatomy_%ld_%s.txt", directoryName, index, suffix );
	file = AbstractFile::open( globals::recordFileType, filename, "w" );
	if( !file )
	{
		fprintf( stderr, "%s: could not open file %s\n", __FUNCTION__, filename );
		return;
	}

	// print the header, with index, fitness, and number of neurons
	file->printf( "brain %ld fitness=%g numneurons+1=%d maxWeight=%g maxBias=%g",
				  index, fitness, dims.numneurons+1, brain::gMaxWeight, brain::gNeuralValues.maxbias );

	cns->dumpAnatomical( file );
	file->printf( "\n" );

	neuralnet->dumpAnatomical( file );
	
	delete file;

	daPrint( "%s: done with anatomy file for %ld\n", __FUNCTION__, index );
}

//---------------------------------------------------------------------------
// Brain::startFunctional
//---------------------------------------------------------------------------
AbstractFile *Brain::startFunctional( long index )
{
	AbstractFile *file;
	char filename[256];

	sprintf( filename, "run/brain/function/incomplete_brainFunction_%ld.txt", index );
	file = AbstractFile::open( globals::recordFileType, filename, "w" );

	if( !file )
	{
		fprintf( stderr, "%s: could not open file %s\n", __FUNCTION__, filename );
		goto bail;
	}

	file->printf( "version 1\n" );

	// print the header, with index (agent number)
	file->printf( "brainFunction %ld", index );	

	// print neuron count, number of input neurons, number of synapses
	neuralnet->startFunctional( file );

	// print timestep born
	file->printf( " %ld", TSimulation::fStep );

	// print organs portion
	cns->startFunctional( file );

	file->printf( "\n" );
	
bail:

	return( file );
}

//---------------------------------------------------------------------------
// Brain::endFunctional
//---------------------------------------------------------------------------
void Brain::endFunctional( AbstractFile *file, float fitness )
{
	if( !file )
		return;

	file->printf( "end fitness = %g\n", fitness );
	delete file;
}

//---------------------------------------------------------------------------
// Brain::writeFunctional
//---------------------------------------------------------------------------
void Brain::writeFunctional( AbstractFile *file )
{
	if( !file )
		return;

	neuralnet->writeFunctional( file );
}

//---------------------------------------------------------------------------
// Brain::Dump
//---------------------------------------------------------------------------
void Brain::Dump(std::ostream& out)
{
#define DIM(NAME) out << dims.NAME << " "
	DIM(numneurons);
	DIM(numsynapses);
	DIM(numgroups);
	DIM(numInputNeurons);
	DIM(numNonInputNeurons);
	DIM(numOutputNeurons);
	DIM(firstOutputNeuron);
	DIM(firstNonInputNeuron);
	DIM(firstInternalNeuron);
#undef DIM

	out << energyuse nl;

	neuralnet->dump( out );
}


//---------------------------------------------------------------------------
// Brain::Load
//---------------------------------------------------------------------------
void Brain::Load(std::istream& in)
{
#define DIM(NAME) in >> dims.NAME
	DIM(numneurons);
	DIM(numsynapses);
	DIM(numgroups);
	DIM(numInputNeurons);
	DIM(numNonInputNeurons);
	DIM(numOutputNeurons);
	DIM(firstOutputNeuron);
	DIM(firstNonInputNeuron);
	DIM(firstInternalNeuron);
#undef DIM

	in >> energyuse;

    InitNeuralNet( 0.0 );

	neuralnet->load( in );
}


//---------------------------------------------------------------------------
// Brain::InitNeuralNet
//---------------------------------------------------------------------------
void Brain::InitNeuralNet( float initial_activation )
{
	neuralnet->init( mygenes,
					 &dims,
					 initial_activation );
}


//---------------------------------------------------------------------------
// Brain::NearestFreeNeuron
//---------------------------------------------------------------------------
short Brain::NearestFreeNeuron(short iin, bool* used, short num, short exclude)
{
    short iout;
    bool tideishigh;
    short hitide = iin;
    short lotide = iin;

//  cout << "*****************************************************" nlf;
//  cout << "iin = " << iin << " , num = " << num << " , exclude = "
//       << exclude nlf;
//  for (short i = 0; i < num; i++)
//      cout << "used[" << i << "] = " << used[i] nl;
//  cout.flush();

    if (iin < (num-1))
    {
        iout = iin + 1;
        tideishigh = true;
    }
    else
    {
        iout = iin - 1;
        tideishigh = false;
    }

    while (used[iout] || (iout == exclude))
    {
//      cout << "iout = " << iout << " , lotide, hitide, tideishigh = "
//           << lotide cms hitide cms tideishigh nlf;
        if (tideishigh)
        {
            hitide = iout;
            if (lotide > 0)
            {
                iout = lotide - 1;
                tideishigh = false;
            }
            else if (hitide < (num-1))
                iout++;
        }
        else
        {
            lotide = iout;
            if (hitide < (num-1))
            {
                iout = hitide + 1;
                tideishigh = true;
            }
            else if (lotide > 0)
                iout--;
        }
//      cout << "new iout = " << iout nlf;
        if ((lotide == 0) && (hitide == (num-1)))
            error(2,"brain::nearestfreeneuron search failed");
    }
    return iout;
}

//---------------------------------------------------------------------------
// Brain::GrowDesignedBrain
//---------------------------------------------------------------------------
void Brain::GrowDesignedBrain( Genome* g )
{
	mygenes = g;
	this->cns = cns;

	ALLOC_GROW_STACK_BUFFERS();

	// This routine builds a very simple, specific brain, to be used for debugging.
	// The number of vision neurons is the same for each component of color (NumColorNeurons).
	// There are no strictly internal neurons; rather inputs are wired directly to outputs.
	// Red input is wired to the Fight response.  Green input is wired to the Eat response.
	// Blue input is wired to the Mate response.  All three are wired to the Move response.
	// The left side of the visual field causes a turn to the left and the right side of the
	// visual field causes a turn to the right; this is true for all colors.
	//
	// It is best if NumColorNeurons is an even number.
	
#define NumColorNeurons 4

#define LeftColorNeurons NumColorNeurons>>1

	// Following helper arrays tell how many neurons are in each input and output group
	// random (1), energy (1), red (NumColorNeurons), green (NumColorNeurons), blue (NumColorNeurons), plus 7 output behaviors (1 each)
	int numIoGroups = mygenes->schema->getMaxGroupCount( NGT_INPUT ) + mygenes->schema->getMaxGroupCount( NGT_OUTPUT );
	int numDesignExcNeurons[numIoGroups];
	int numDesignInhNeurons[numIoGroups];

	for( int i = 0; i < numIoGroups; i++ )
	{
		numDesignExcNeurons[i] = 1;
		numDesignInhNeurons[i] = 1;
	}

	for( GeneVector::const_iterator
			 it = mygenes->schema->getAll( Gene::NEURGROUP ).begin(),
			 end = mygenes->schema->getAll( Gene::NEURGROUP ).end();
		 it != end;
		 it++ )
	{
		NeurGroupGene *g = (*it)->to_NeurGroup();

		if( g->isMember(NGT_INPUT) )
		{
			string name = g->name;

			if( name == "Red" || name == "Green" || name == "Blue" )
			{
				int i = mygenes->schema->getFirstGroup( g );

				numDesignExcNeurons[i] = NumColorNeurons;
				numDesignInhNeurons[i] = NumColorNeurons;
			}
		}
	}

	
	// we only have input and output neural groups in this simplifed brain
	dims.numgroups = cns->getNerveCount();

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "****************************************************" nlf;
		cout << "Starting a new brain with numneurgroups = " << dims.numgroups nlf;
	}
#endif

	dims.numInputNeurons = 0;
	int index = 0;

	for( NervousSystem::nerve_iterator
			 it = cns->begin_nerve(),
			 end = cns->end_nerve();
		 it != end;
		 it++ )
	{
		Nerve *nerve = *it;
		int numneurons = numDesignExcNeurons[nerve->igroup];

		if( nerve->type == Nerve::INPUT )
		{
			firsteneur[nerve->igroup] = dims.numInputNeurons;
			firstineur[nerve->igroup] = dims.numInputNeurons;

			dims.numInputNeurons += numneurons;
		}
		
		nerve->config( numneurons, index );

		index += numneurons;

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "group " << nerve->igroup << " has " << numDesignExcNeurons[nerve->igroup] << " neurons" nlf;
#endif
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = "
			 << cns->get("red")->getNeuronCount() cms cns->get("green")->getNeuronCount() cms cns->get("blue")->getNeuronCount() nlf;;
	}
#endif
	
	dims.firstNonInputNeuron = dims.numInputNeurons;

	dims.numsynapses = 0;
	dims.numNonInputNeurons = 0;
	
	short i, j, ii;
	for(i = cns->getNerveCount( Nerve::INPUT ); i < dims.numgroups; i++)
	{
		// For this simplified, test brain, there are no purely internal neural groups,
		// so all of these will be output groups, which always consist of a single neuron,
		// that can play the role of both an excitatory and an inhibitory neuron.
		firsteneur[i] = dims.numInputNeurons + dims.numNonInputNeurons;
		firstineur[i] = dims.numInputNeurons + dims.numNonInputNeurons;
		dims.numNonInputNeurons++;
		
#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "group " << i << " has " << numDesignExcNeurons[i] << " e-neurons" nlf;
			cout << "  and " << i << " has " << numDesignInhNeurons[i] << " i-neurons" nlf;
		}
#endif

		// Since we are only dealing with output groups, there is only one neuron in each group,
		// so the synapse count is just the number of neurons providing input to this group.
		// But since the presynaptic groups are all input groups, we're going to let them have
		// both excitatory and inhibitory connections, and then we'll set the synaptic values
		// to do the right thing.  (So count connections from the incoming neurons twice.)
		for( j = 0; j < cns->getNerveCount( Nerve::INPUT ); j++ )
		{
			dims.numsynapses += numDesignExcNeurons[j];
			dims.numsynapses += numDesignInhNeurons[j];	// see comment above
			if( j > 4 )	// oops!  just a sanity check
			{
				printf( "%s: ERROR initializing numsynapses, invalid pre-synaptic group (%d)\n", __FUNCTION__, j );
				exit( 1 );
			}
#if DebugBrainGrow
			if( DebugBrainGrowPrint )
			{
				cout << "  from " << j << " to " << i << " there are "
					 << numDesignExcNeurons[j] << " e-e synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << numDesignInhNeurons[j] << " i-e synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << 0 << " e-i synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << 0 << " i-i synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << numDesignExcNeurons[j] + numDesignInhNeurons[j] << " total synapses" nlf;
			}
#endif
		}
	}
	
    dims.numneurons = dims.numNonInputNeurons + dims.numInputNeurons;
    if (dims.numneurons > brain::gNeuralValues.maxneurons)
        error(2,"numneurons (",dims.numneurons,") > maxneurons (", brain::gNeuralValues.maxneurons,") in brain::grow");
        
    if (dims.numsynapses > brain::gNeuralValues.maxsynapses)
        error(2,"numsynapses (",dims.numsynapses,") > maxsynapses (", brain::gNeuralValues.maxsynapses,") in brain::grow");

	dims.numOutputNeurons = cns->getNeuronCount( Nerve::OUTPUT );
	dims.firstOutputNeuron = cns->get( Nerve::OUTPUT, 0 )->getIndex();
	dims.firstInternalNeuron = dims.firstOutputNeuron + dims.numOutputNeurons;

	InitNeuralNet( 0.0 );
	
    short ineur, jneur, nneuri, nneurj, joff, disneur;
    short isyn, newsyn;
    long  nsynji;
    float nsynjiperneur;
    long numsyn = 0;
    short numneur = dims.numInputNeurons;
    float tdji;

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "  Initializing input neurons:" nlf;
#endif

    for (i = 0, ineur = 0; i < brain::gNeuralValues.numinputneurgroups; i++)		//for all input neural groups
    {
        for (j = 0; j < numDesignExcNeurons[i]; j++, ineur++)
        {
			neuralnet->set_neuron( ineur,
								   i,
								   0.0 );
        }
    }

    for (i = brain::gNeuralValues.numinputneurgroups; i < dims.numgroups; i++)		//for all behavior (and the non-existent internal) neural groups
    {
#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "For group " << i << ":" nlf;
#endif

        float groupbias;
		
		if( i == brain::gNeuralValues.numinputneurgroups + 4 )	// yaw group/neuron		
			groupbias = 0.5;	// straight ahead (< 0.5 is turn one way, > 0.5 is turn other way)
		else
			groupbias = 0.0;	// no bias	// g->Bias(i);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  groupbias = " << groupbias nlf;
#endif

		neuralnet->set_groupblrate( i,
									0.0 );

        for (j = 0; j < dims.numgroups; j++)
        {
            eeremainder[j] = 0.0;
            eiremainder[j] = 0.0;
            iiremainder[j] = 0.0;
            ieremainder[j] = 0.0;

			for( SynapseTypeList::const_iterator
					 it = g->getSynapseTypes().begin(),
					 end = g->getSynapseTypes().end();
				 it != end;
				 it++ )
			{
				SynapseType *stype = *it;

				neuralnet->set_grouplrate( stype,
										   j,
										   i,
										   0.0 );
			}
        }

        // setup all e-neurons for this group
        nneuri = numDesignExcNeurons[i];

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  Setting up " << nneuri << " e-neurons" nlf;
#endif
		
		short ini;	
        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firsteneur[i];

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
				cout << "  For ini, ineur = " << ini cms ineur << ":" nlf;
#endif

			neuralnet->set_neuron( ineur, i, groupbias, numsyn );

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
				cout << "    Setting up e-e connections:" nlf;
#endif

            // setup all e-e connections for this e-neuron
            for (j = 0; j < dims.numgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynji = 0;
				}
				else
				{
					nneurj = numDesignExcNeurons[j];	// g->numeneur(j);
					nsynji = numDesignExcNeurons[j];	// g->numeesynapses(i,j);
				}

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      From group " << j nlf;
					cout << "      with nneurj, (old)eeremainder = "
						 << nneurj cms eeremainder[j] nlf;
				}
#endif

                nsynjiperneur = float(nsynji)/float(nneuri);
                newsyn = short(nsynjiperneur + eeremainder[j] + 1.e-5);
                eeremainder[j] += nsynjiperneur - newsyn;
                tdji = 0.0;	// no topological distortion // g->eetd(i,j);

                joff = short((float(ini) / float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      and nsynji, nsynjiperneur, newsyn = "
						 << nsynji cms nsynjiperneur cms newsyn nlf;
					cout << "      and (new)eeremainder, tdji, joff = "
						 << eeremainder[j] cms tdji cms joff nlf;
				}
#endif

                if ((joff + newsyn) > nneurj)
                {
                    error(2,"Illegal architecture generated: ",
                        "more e-e synapses from group ",j,
                        " to group ",i,
                        " than there are e-neurons in group ",j);
                }

                if (newsyn > 0)
                {
                    for (ii = 0; ii < nneurj; ii++)
                        neurused[ii] = false;
				}
				
                for (isyn = 0; isyn < newsyn; isyn++)
                {
                    if (rng->drand() < tdji)
                    {
                        disneur = short(nint(rng->range(-0.5,0.5)*tdji*nneurj));
                        jneur = isyn + joff + disneur;
                        
                        if (jneur < 0)
                            jneur += nneurj;
                        else if (jneur >= nneurj)
                            jneur -= nneurj;
                    }
                    else
                    {
                        jneur = isyn + joff;
					}                      

                    if (((jneur+firsteneur[j]) == ineur) // same neuron or
                        || neurused[jneur] ) // already connected to this one
                    {
                        if (i == j) // same group and neuron type (because we're doing e->e currently)
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur - firsteneur[j]);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur,&neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firsteneur[j];

					neuralnet->set_synapse( numsyn,
											jneur,
											ineur,
											ineur == jneur ? 0.0 : DesignedEfficacy(i, j, isyn, kSynapseTypeEE) );

                    numsyn++;
                }
            }

            // setup all i-e connections for this e-neuron

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
				cout << "    Setting up i-e connections:" nlf;
#endif

            for (j = 0; j < dims.numgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynji = 0;
				}
				else
				{
					nneurj = numDesignInhNeurons[j];	// g->numineur(j);
					nsynji = numDesignInhNeurons[j];	// g->numiesynapses(i,j);
				}

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      From group " << j nlf;
					cout << "      with nneurj, (old)ieremainder = "
						 << nneurj cms ieremainder[j] nlf;
				}
#endif

                nsynjiperneur = float(nsynji)/float(nneuri);
                newsyn = short(nsynjiperneur + ieremainder[j] + 1.e-5);
                ieremainder[j] += nsynjiperneur - newsyn;
                tdji = 0.0;	// no topological distortion	// g->ietd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj)
                     - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      and nsynji, nsynjiperneur, newsyn = "
						 << nsynji cms nsynjiperneur cms newsyn nlf;
					cout << "      and (new)ieremainder, tdji, joff = "
						 << ieremainder[j] cms tdji cms joff nlf;
				}
#endif

                if ((joff+newsyn) > nneurj)
                {
                    error(2,"Illegal architecture generated: ",
                        "more i-e synapses from group ",j,
                        " to group ",i,
                        " than there are i-neurons in group ",j);
                }

                if (newsyn > 0)
                {
                    for (ii = 0; ii < nneurj; ii++)
                        neurused[ii] = false;
				}
				
                for (isyn = 0; isyn < newsyn; isyn++)
                {
                    if (rng->drand() < tdji)
                    {
                        disneur = short(nint(rng->range(-0.5,0.5)*tdji*nneurj));
                        jneur = isyn + joff + disneur;
                        if (jneur < 0)
                            jneur += nneurj;
                        else if (jneur >= nneurj)
                            jneur -= nneurj;
                    }
                    else
                    {
                        jneur = isyn + joff;
					}                     

                    if ( ((jneur+firstineur[j]) == ineur) // same neuron or
                        || neurused[jneur] ) // already connected to this one
                    {
                        if( (i == j) && IsOutputNeuralGroup( i ) )//same & output group
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur - firstineur[j]);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur,&neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firstineur[j];

					neuralnet->set_synapse( numsyn,
											-jneur, // - denotes inhibitory
											ineur,  // + denotes excitatory
											ineur == jneur ? 0.0 : min(-1.e-10, (double) DesignedEfficacy( i, j, isyn, kSynapseTypeIE )) );

                    numsyn++;
                }
            }

			neuralnet->set_neuron_endsynapses( ineur, numsyn );
            numneur++;
        }

        // setup all i-neurons for this group

        if( IsOutputNeuralGroup( i ) )
            nneuri = 0;  // output/behavior neurons are e-only postsynaptically
        else
            nneuri = numDesignInhNeurons[i];	// g->numineur(i);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  Setting up " << nneuri << " i-neurons" nlf;
#endif

        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firstineur[i];

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
			{
				cout << "  For ini, ineur = "
					 << ini cms ineur << ":" nlf;
			}
#endif

			neuralnet->set_neuron( ineur,
								   i,
								   groupbias,
								   numsyn );

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
				cout << "    Setting up e-i connections:" nlf;
#endif

            // setup all e-i connections for this i-neuron

            for (j = 0; j < dims.numgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynji = 0;
				}
				else
				{
					nneurj = numDesignExcNeurons[j];	// g->numeneur(j);
					nsynji = numDesignExcNeurons[j];	// g->numeisynapses(i, j);
				}

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      From group " << j nlf;
					cout << "      with nneurj, (old)eiremainder = "
						 << nneurj cms eiremainder[j] nlf;
				}
#endif

                nsynjiperneur = float(nsynji) / float(nneuri);
                newsyn = short(nsynjiperneur + eiremainder[j] + 1.e-5);
                eiremainder[j] += nsynjiperneur - newsyn;
                tdji = 0.0;	// no topological distortion	// g->eitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      and nsynji, nsynjiperneur, newsyn = "
						 << nsynji cms nsynjiperneur cms newsyn nlf;
					cout << "      and (new)eiremainder, tdji, joff = "
						 << eiremainder[j] cms tdji cms joff nlf;
				}
#endif

                if ((joff+newsyn) > nneurj)
                {
                    error(2,"Illegal architecture generated: ",
                        "more e-i synapses from group ",j,
                        " to group ",i,
                        " than there are e-neurons in group ",j);
                }

                if (newsyn > 0)
                {
                    for (ii = 0; ii < nneurj; ii++)
                        neurused[ii] = false;
				}
				
                for (isyn = 0; isyn < newsyn; isyn++)
                {
                    if (rng->drand() < tdji)
                    {
                        disneur = short(nint(rng->range(-0.5,0.5)*tdji*nneurj));
                        jneur = isyn + joff + disneur;
                        if (jneur < 0)
                            jneur += nneurj;
                        else if (jneur >= nneurj)
                            jneur -= nneurj;
                    }
                    else
                    {
                        jneur = isyn + joff;
					}
					
                    if ( ((jneur+firsteneur[j]) == ineur) // same neuron or
                        || neurused[jneur] ) // already connected to this one
                    {
                        if( (i == j) && IsOutputNeuralGroup( i ) )	//same & output group
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur - firsteneur[j]);
                        else
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firsteneur[j];

					neuralnet->set_synapse( numsyn,
											jneur,  // + denotes excitatory
											-ineur, // - denotes inhibitory
											ineur == jneur ? 0.0 : DesignedEfficacy(i, j, isyn, kSynapseTypeEI) );

                    numsyn++;
                }
            }

            // setup all i-i connections for this i-neuron
            for (j = 0; j < dims.numgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynji = 0;
				}
				else
				{
					nneurj = numDesignInhNeurons[j];	// g->numineur(j);
					nsynji = numDesignInhNeurons[j];	// g->numiisynapses(i,j);
				}

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      From group " << j nlf;
					cout << "      with nneurj, (old)iiremainder = "
						 << nneurj cms iiremainder[j] nlf;
				}
#endif

                nsynjiperneur = float(nsynji)/float(nneuri);
                newsyn = short(nsynjiperneur + iiremainder[j] + 1.e-5);
                iiremainder[j] += nsynjiperneur - newsyn;
                tdji = 0.0;	// no topological distortion	// g->iitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
				if( DebugBrainGrowPrint )
				{
					cout << "      and nsynji, nsynjiperneur, newsyn = "
						 << nsynji cms nsynjiperneur cms newsyn nlf;
					cout << "      and (new)iiremainder, tdji, joff = "
						 << iiremainder[j] cms tdji cms joff nlf;
				}
#endif

                if ((joff+newsyn) > nneurj)
                {
                    error(2,"Illegal architecture generated: ",
                        "more i-i synapses from group ",j,
                        " to group ",i,
                        " than there are i-neurons in group ",j);
                }

                if (newsyn > 0)
                {
                    for (ii = 0; ii < nneurj; ii++)
                        neurused[ii] = false;
				}
				
                for (isyn = 0; isyn < newsyn; isyn++)
                {
                    if (rng->drand() < tdji)
                    {
                        disneur = short(nint(rng->range(-0.5,0.5)*tdji*nneurj));
                        jneur = isyn + joff + disneur;
                        if (jneur < 0)
                            jneur += nneurj;
                        else if (jneur >= nneurj)
                            jneur -= nneurj;
                    }
                    else
                    {
                        jneur = isyn + joff;
					}
					
                    if (((jneur+firstineur[j]) == ineur) // same neuron or
                        || neurused[jneur] ) // already connected to this one
                    {
                        if (i == j) // same group and neuron type
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur - firstineur[j]);
                        else
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firstineur[j];

					neuralnet->set_synapse( numsyn,
											-jneur, // - denotes inhibitory
											-ineur, // - denotes inhibitory
											ineur == jneur ? 0.0 : min(-1.e-10, (double) DesignedEfficacy( i, j, isyn, kSynapseTypeII )) );

                    numsyn++;
                }
            }

			neuralnet->set_neuron_endsynapses( ineur, numsyn );
            numneur++;
        }
    }

    if (numneur != (dims.numneurons))
        error(2,"Bad neural architecture, numneur (",numneur,") not equal to numneurons (",dims.numneurons,")");

    if (numsyn != (dims.numsynapses))
        error(2,"Bad neural architecture, numsyn (",numsyn,") not equal to numsynapses (",dims.numsynapses,")");

    energyuse = brain::gNeuralValues.maxneuron2energy * float(dims.numneurons) / float(brain::gNeuralValues.maxneurons)
              + brain::gNeuralValues.maxsynapse2energy * float(dims.numsynapses) / float(brain::gNeuralValues.maxsynapses);

    debugcheck(" after setting up brain architecture ");
}


//---------------------------------------------------------------------------
// Brain::DesignedEfficacy
//---------------------------------------------------------------------------
#define DebugDesignedEfficacy 0
#if DebugDesignedEfficacy
	#define dePrint( x... ) printf( x )
#else
	#define dePrint( x... )
#endif
float Brain::DesignedEfficacy( short toGroup, short fromGroup, short isyn, int synapseType )
{
	float efficacy = 0.0;	// may be set < 0.0 for inhibitory connections in caller

	Gene *gto = mygenes->schema->getGroupGene( toGroup );
	Gene *gfrom = mygenes->schema->getGroupGene( fromGroup );
	string to = gto->name;
	string from = gfrom->name;

	if( to == "Eat" )
	{
		if( (from == "Green") && (synapseType == kSynapseTypeEE) )	// green vision
		{
			dePrint( "%s: setting efficacy to %g for green vision connecting to the eat behavior\n", __FUNCTION__, brain::gMaxWeight );
			efficacy = brain::gMaxWeight;
		}
	}
	else if( to == "Mate" )
	{
		if( (from == "Blue") && (synapseType == kSynapseTypeEE) )	// blue vision
		{
			dePrint( "%s: setting efficacy to %g for blue vision connecting to the mate behavior\n", __FUNCTION__, brain::gMaxWeight );
			efficacy = brain::gMaxWeight;
		}
	}
	else if( to == "Fight" )
	{
		if( (from == "Red") && (synapseType == kSynapseTypeEE) )	// red vision
		{
			dePrint( "%s: setting efficacy to %g for red vision connecting to the fight behavior\n", __FUNCTION__, brain::gMaxWeight );
			efficacy = brain::gMaxWeight;
		}
	}
	else if( to == "Speed" )
	{
		if( ((from == "Red") || (from == "Green") || (from == "Blue")) && (synapseType == kSynapseTypeEE) )	// any vision
		{
			dePrint( "%s: setting efficacy to %g for any vision connecting to the move behavior\n", __FUNCTION__, brain::gMaxWeight );
			efficacy = brain::gMaxWeight;
		}
	}
	else if( to == "Yaw" )
	{
		if( from == "Red" )
		{
			if( (isyn < LeftColorNeurons) && ((synapseType == kSynapseTypeIE) || (synapseType == kSynapseTypeII)) )
			{
				dePrint( "%s: setting efficacy to %g for left red vision connecting to the turn behavior\n", __FUNCTION__, -brain::gMaxWeight );
				efficacy = -brain::gMaxWeight;	// input on left forces turn to right (negative yaw)
			}
			else if( (isyn >= LeftColorNeurons) && ((synapseType == kSynapseTypeEE) || (synapseType == kSynapseTypeEI)) )
			{
				dePrint( "%s: setting efficacy to %g for right red vision connecting to the turn behavior\n", __FUNCTION__, brain::gMaxWeight );
				efficacy = brain::gMaxWeight;		// input on right forces turn to left (positive yaw)
			}
		}
		else if( from == "Green" )
		{
			if( (isyn < LeftColorNeurons) && ((synapseType == kSynapseTypeEE) || (synapseType == kSynapseTypeEI)) )
			{
				dePrint( "%s: setting efficacy to %g for left green vision connecting to the turn behavior\n", __FUNCTION__, brain::gMaxWeight * 0.5 );
				efficacy = brain::gMaxWeight * 0.5;	// input on left forces turn to left (positive yaw)
			}
			else if( (isyn >= LeftColorNeurons) && ((synapseType == kSynapseTypeIE) || (synapseType == kSynapseTypeII)) )
			{
				dePrint( "%s: setting efficacy to %g for right green vision connecting to the turn behavior\n", __FUNCTION__, -brain::gMaxWeight * 0.5 );
				efficacy = -brain::gMaxWeight * 0.5;		// input on right green forces turn to right (negative yaw)
			}
		}
		else if( from == "Blue" )
		{
			if( (isyn < LeftColorNeurons) && ((synapseType == kSynapseTypeEE) || (synapseType == kSynapseTypeEI)) )
			{
				dePrint( "%s: setting efficacy to %g for left blue vision connecting to the turn behavior\n", __FUNCTION__, brain::gMaxWeight * 0.75 );
				efficacy = brain::gMaxWeight * 0.75;	// input on left forces turn to left (positive yaw)
			}
			else if( (isyn >= LeftColorNeurons) && ((synapseType == kSynapseTypeIE) || (synapseType == kSynapseTypeII)) )
			{
				dePrint( "%s: setting efficacy to %g for right blue vision connecting to the turn behavior\n", __FUNCTION__, -brain::gMaxWeight * 0.75 );
				efficacy = -brain::gMaxWeight * 0.75;		// input on right blue forces turn to right (negative yaw)
			}
		}
	}	
	
	return( efficacy );
}

//---------------------------------------------------------------------------
// Brain::GrowSynapses
//---------------------------------------------------------------------------
void Brain::GrowSynapses( Genome *g,
						  int groupIndex_to,
						  short neuronCount_to,
						  float *remainder,
						  short neuronLocalIndex_to,
						  short neuronIndex_to,
						  short *firstneur,
						  long &synapseCount_brain,
						  SynapseType *synapseType )
{
#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "    Setting up " << synapseType->name << " connections:" nlf;
#endif

	int sign_to = synapseType->nt_to == EXCITATORY ? 1 : -1;
	int sign_from = synapseType->nt_from == EXCITATORY ? 1 : -1;

	RandomNumberGenerator *td_rng;
	Gene *td_seedGene;
	if( brain::gNeuralValues.enableTopologicalDistortionRngSeed )
	{
		td_rng = RandomNumberGenerator::create( RandomNumberGenerator::TOPOLOGICAL_DISTORTION );
		td_seedGene = g->gene( "TopologicalDistortionRngSeed" );
	}
	else
	{
		td_rng = this->rng;
		td_seedGene = NULL;
	}
	RandomNumberGenerator *weight_rng;
	Gene *weight_seedGene;
	if( brain::gEnableInitWeightRngSeed )
	{
		weight_rng = RandomNumberGenerator::create( RandomNumberGenerator::INIT_WEIGHT );
		weight_seedGene = g->gene( "InitWeightRngSeed" );
	}
	else
	{
		weight_rng = this->rng;
		weight_seedGene = NULL;
	}

	for (int groupIndex_from = 0; groupIndex_from < dims.numgroups; groupIndex_from++)
	{
		int neuronCount_from = g->getNeuronCount(synapseType->nt_from, groupIndex_from);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "      From group " << groupIndex_from nlf;
			cout << "      with neuronCount_from, (old)" << synapseType->name << "remainder = "
				 << neuronCount_from cms remainder[groupIndex_from] nlf;
		}
#endif

		int synapseCount_fromto = g->getSynapseCount( synapseType,
													  groupIndex_from,
													  groupIndex_to );
		float nsynjiperneur = float(synapseCount_fromto)/float(neuronCount_to);
		int synapseCount_new = short(nsynjiperneur + remainder[groupIndex_from] + 1.e-5);
		remainder[groupIndex_from] += nsynjiperneur - synapseCount_new;
		float td_fromto = g->get( g->TOPOLOGICAL_DISTORTION,
								  synapseType,
								  groupIndex_from,
								  groupIndex_to );
		if( brain::gNeuralValues.enableTopologicalDistortionRngSeed )
		{
			long td_seed = g->get( td_seedGene,
								   synapseType,
								   groupIndex_from,
								   groupIndex_to );
			td_rng->seed( td_seed );
		}
		if( brain::gEnableInitWeightRngSeed )
		{
			long weight_seed = g->get( weight_seedGene,
									   synapseType,
									   groupIndex_from,
									   groupIndex_to );
			weight_rng->seed( weight_seed );
		}

		// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
		// the group as opposed to the entire neuron array.
		int neuronLocalIndex_fromBase = short((float(neuronLocalIndex_to) / float(neuronCount_to)) * float(neuronCount_from) - float(synapseCount_new) * 0.5);
		neuronLocalIndex_fromBase = max<short>(0, min<short>(neuronCount_from - synapseCount_new, neuronLocalIndex_fromBase));

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "      and synapseCount_fromto, nsynjiperneur, synapseCount_new = "
				 << synapseCount_fromto cms nsynjiperneur cms synapseCount_new nlf;
			cout << "      and (new)" << synapseType->name << "remainder, td_fromto, neuronLocalIndex_fromBase = "
				 << remainder[groupIndex_from] cms td_fromto cms neuronLocalIndex_fromBase nlf;
		}
#endif

		if ((neuronLocalIndex_fromBase + synapseCount_new) > neuronCount_from)
		{
			char msg[128];
			sprintf( msg, "more %s synapses from group ", synapseType->name.c_str() );
			error(2,"Illegal architecture generated: ",
				  msg, groupIndex_from,
				  " to group ",groupIndex_to,
				  " than there are i-neurons in group ",groupIndex_from);
		}

		bool neurused[neuronCount_from];
		memset( neurused, 0, sizeof(neurused) );
				
		for (int isyn = 0; isyn < synapseCount_new; isyn++)
		{
			// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
			// the group as opposed to the entire neuron array.
			int neuronLocalIndex_from;

			if (td_rng->drand() < td_fromto)
			{
				short distortion = short(nint(td_rng->range(-0.5,0.5)*td_fromto*neuronCount_from));
				neuronLocalIndex_from = isyn + neuronLocalIndex_fromBase + distortion;
                        
				if (neuronLocalIndex_from < 0)
					neuronLocalIndex_from += neuronCount_from;
				else if (neuronLocalIndex_from >= neuronCount_from)
					neuronLocalIndex_from -= neuronCount_from;
			}
			else
			{
				neuronLocalIndex_from = isyn + neuronLocalIndex_fromBase;
			}                      

			if (((neuronLocalIndex_from+firstneur[groupIndex_from]) == neuronIndex_to) // same neuron or
				|| neurused[neuronLocalIndex_from] ) // already connected to this one
			{
				if ( (groupIndex_to == groupIndex_from) // same group
					 && (synapseType->nt_from == synapseType->nt_to // same neuron type (I or E)
						 || IsOutputNeuralGroup(groupIndex_to)) )
					neuronLocalIndex_from = NearestFreeNeuron( neuronLocalIndex_from,
															   &neurused[0],
															   neuronCount_from,
															   neuronIndex_to - firstneur[groupIndex_from] );
				else
					neuronLocalIndex_from = NearestFreeNeuron( neuronLocalIndex_from,
															   &neurused[0],
															   neuronCount_from,
															   neuronLocalIndex_from);
			}

			neurused[neuronLocalIndex_from] = true;

			int neuronIndex_from = neuronLocalIndex_from + firstneur[groupIndex_from];

			// We should never have a self-synapsing neuron.
			assert( neuronIndex_from != neuronIndex_to );

			float efficacy;
			if( sign_from < 0 )
			{
				efficacy = min(-1.e-10, -weight_rng->range(initminweight, brain::gInitMaxWeight));
			}
			else
			{
				efficacy = weight_rng->range(initminweight, brain::gInitMaxWeight);
			}				

			neuralnet->set_synapse( synapseCount_brain,
									neuronIndex_from * sign_from,
									neuronIndex_to * sign_to,
									efficacy );

			synapseCount_brain++;
		}
	}

	if( brain::gNeuralValues.enableTopologicalDistortionRngSeed )
	{
		RandomNumberGenerator::dispose( td_rng );
	}
	if( brain::gEnableInitWeightRngSeed )
	{
		RandomNumberGenerator::dispose( weight_rng );
	}
}

//---------------------------------------------------------------------------
// Brain::Grow
//---------------------------------------------------------------------------
void Brain::Grow( Genome* g )
{
    debugcheck( "(brain) on entry" );

#if DesignerBrains
	GrowDesignedBrain( g );
	return;
#endif

	ALLOC_GROW_STACK_BUFFERS();
	
    mygenes = g;

    dims.numgroups = g->getGroupCount(NGT_ANY);
	dims.numgroupsWithNeurons = 0;
	for( int i = 0; i < dims.numgroups; i++ )
	{
		if( g->getNeuronCount(i) > 0 )
		{
			dims.numgroupsWithNeurons++;
		}
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "****************************************************" nlf;
		cout << "Starting a new brain with numneurgroups = " << dims.numgroups nlf;
	}
#endif

	dims.numInputNeurons = 0;
	dims.numNonInputNeurons = 0;

	// ---
	// --- Configure Input/Output Neurons
	// ---
	int index = 0;
	for( NervousSystem::nerve_iterator
			 it = cns->begin_nerve(),
			 end = cns->end_nerve();
		 it != end;
		 it++ )
	{
		Nerve *nerve = *it;

		int numneurons = g->getNeuronCount( EXCITATORY, nerve->igroup );
		nerve->config( numneurons, index );

        firsteneur[nerve->igroup] = index;
        firstineur[nerve->igroup] = index; // input/output neurons double as e & i

		if( nerve->type == Nerve::INPUT )
		{
			dims.numInputNeurons += numneurons;
		}
		else
		{
			dims.numNonInputNeurons += numneurons;
		}

		index += numneurons;

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "group " << nerve->igroup << " (" << nerve->name << ") has " << nerve->getNeuronCount() << " neurons" nlf;
#endif
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "numinputneurons = " << dims.numInputNeurons nlf;
#endif
    dims.firstNonInputNeuron = dims.numInputNeurons;

	dims.numOutputNeurons = cns->getNeuronCount( Nerve::OUTPUT );

	dims.firstOutputNeuron = cns->get( Nerve::OUTPUT, 0 )->getIndex();
	dims.firstInternalNeuron = dims.firstOutputNeuron + dims.numOutputNeurons;

	// ---
	// --- Configure Internal Groups
	// ---
    for( int i = cns->getNerveCount();
		 i < dims.numgroups;
		 i++ )
    {
		firsteneur[i] = dims.numInputNeurons + dims.numNonInputNeurons;
		dims.numNonInputNeurons += g->getNeuronCount(EXCITATORY, i);
		firstineur[i] = dims.numInputNeurons + dims.numNonInputNeurons;
		dims.numNonInputNeurons += g->getNeuronCount(INHIBITORY, i);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "group " << i << " has " << g->getNeuronCount(EXCITATORY, i) << " e-neurons" nlf;
			cout << "  and " << i << " has " << g->getNeuronCount(INHIBITORY, i) << " i-neurons" nlf;
		}
#endif
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = "
			 << cns->get("red")->getNeuronCount() cms cns->get("green")->getNeuronCount() cms cns->get("blue")->getNeuronCount() nlf;;
	}
#endif

	// ---
	// --- Count Synapses
	// ---
    dims.numsynapses = 0;

    for( int i = cns->getNerveCount( Nerve::INPUT );
		 i < dims.numgroups;
		 i++ )
    {
#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "group " << i << " has " << g->getNeuronCount(EXCITATORY, i) << " e-neurons" nlf;
			cout << "  and " << i << " has " << g->getNeuronCount(INHIBITORY, i) << " i-neurons" nlf;
		}
#endif
        for (int j = 0; j < dims.numgroups; j++)
        {
            dims.numsynapses += g->getSynapseCount(j,i);

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
			{
				cout << "  from " << j << " to " << i << " there are "
					 << g->getSynapseCount(g->EE,j,i) << " e-e synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << g->getSynapseCount(g->IE,j,i) << " i-e synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << g->getSynapseCount(g->EI,j,i) << " e-i synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << g->getSynapseCount(g->II,j,i) << " i-i synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << g->getSynapseCount(j,i) << " total synapses" nlf;
			}
#endif

        }
    }
    
    dims.numneurons = dims.numNonInputNeurons + dims.numInputNeurons;
	if( dims.numneurons > brain::gNeuralValues.maxneurons )
		error( 2, "numneurons (", dims.numneurons, ") > maxneurons (", brain::gNeuralValues.maxneurons, ") in brain::grow" );
        
	if( dims.numsynapses > brain::gNeuralValues.maxsynapses )
		error( 2, "numsynapses (", dims.numsynapses, ") > maxsynapses (", brain::gNeuralValues.maxsynapses, ") in brain::grow" );

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "numneurons = " << dims.numneurons << "  (of " << brain::gNeuralValues.maxneurons pnlf;
		cout << "numsynapses = " << dims.numsynapses << "  (of " << brain::gNeuralValues.maxsynapses pnlf;
	}
#endif

	// ---
	// --- Allocate Neural Net
	// ---
    debugcheck( "before allocating brain memory" );

	InitNeuralNet( 0.1 );	// lsy? - why is this initializing activations to 0.1?

    debugcheck( "after allocating brain memory" );

    long numsyn = 0;
    short numneur = dims.numInputNeurons;

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "  Initializing input neurons:" nlf;
#endif

	// ---
	// --- Initialize Input Neuron Activations
	// ---
    for (int i = 0, ineur = 0; i < brain::gNeuralValues.numinputneurgroups; i++)
    {
        for (int j = 0; j < g->getNeuronCount(EXCITATORY, i); j++, ineur++)
        {
			neuralnet->set_neuron( ineur,
								   i,
								   0.0 );
        }
    }

	// ---
	// --- Grow Synapses
	// ---
    for (int groupIndex_to = brain::gNeuralValues.numinputneurgroups; groupIndex_to < dims.numgroups; groupIndex_to++)
    {
#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "For group " << groupIndex_to << ":" nlf;
#endif

        float groupbias = g->get(g->BIAS,groupIndex_to);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  groupbias = " << groupbias nlf;
#endif

		neuralnet->set_groupblrate( groupIndex_to,
									g->get(g->BIAS_LEARNING_RATE, groupIndex_to) );

        for (int groupIndex_from = 0; groupIndex_from < dims.numgroups; groupIndex_from++)
        {
            eeremainder[groupIndex_from] = 0.0;
            eiremainder[groupIndex_from] = 0.0;
            iiremainder[groupIndex_from] = 0.0;
            ieremainder[groupIndex_from] = 0.0;

			// synapseTypes = EE, EI, IE, II
			citfor( SynapseTypeList, g->getSynapseTypes(), it )
			{
				SynapseType *synapseType = *it;

				neuralnet->set_grouplrate( synapseType,
										   groupIndex_from,
										   groupIndex_to,
										   g->get(g->LEARNING_RATE,
												  synapseType,
												  groupIndex_from,
												  groupIndex_to) );
			}
        }

        // setup all e-neurons for this group
        int neuronCount_to = g->getNeuronCount(EXCITATORY, groupIndex_to);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  Setting up " << neuronCount_to << " e-neurons" nlf;
#endif


		// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
		// the group as opposed to the entire neuron array.
        for (int neuronLocalIndex_to = 0; neuronLocalIndex_to < neuronCount_to; neuronLocalIndex_to++)
        {
            int neuronIndex_to = neuronLocalIndex_to + firsteneur[groupIndex_to];

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
				cout << "  For neuronLocalIndex_to, neuronIndex_to = "
					 << neuronLocalIndex_to cms neuronIndex_to << ":" nlf;
#endif

			neuralnet->set_neuron( neuronIndex_to, groupIndex_to, groupbias, numsyn );

			GrowSynapses( g,
						  groupIndex_to,
						  neuronCount_to,
						  eeremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firsteneur,
						  numsyn,
						  g->EE );

			GrowSynapses( g,
						  groupIndex_to,
						  neuronCount_to,
						  ieremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firstineur,
						  numsyn,
						  g->IE );

			neuralnet->set_neuron_endsynapses( neuronIndex_to, numsyn );
            numneur++;
        }

        // setup all i-neurons for this group

        if( IsOutputNeuralGroup( groupIndex_to ) )
            neuronCount_to = 0;  // output/behavior neurons are e-only postsynaptically
        else
            neuronCount_to = g->getNeuronCount(INHIBITORY,  groupIndex_to );

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  Setting up " << neuronCount_to << " i-neurons" nlf;
#endif

		// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
		// the group as opposed to the entire neuron array.
        for (int neuronLocalIndex_to = 0; neuronLocalIndex_to < neuronCount_to; neuronLocalIndex_to++)
        {
            int neuronIndex_to = neuronLocalIndex_to + firstineur[groupIndex_to];

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
			{
				cout << "  For neuronLocalIndex_to, neuronIndex_to = "
					 << neuronLocalIndex_to cms neuronIndex_to << ":" nlf;
			}
#endif

			neuralnet->set_neuron( neuronIndex_to,
								   groupIndex_to,
								   groupbias,
								   numsyn );

			GrowSynapses( g,
						  groupIndex_to,
						  neuronCount_to,
						  eiremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firsteneur,
						  numsyn,
						  g->EI );

			GrowSynapses( g,
						  groupIndex_to,
						  neuronCount_to,
						  iiremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firstineur,
						  numsyn,
						  g->II );

			neuralnet->set_neuron_endsynapses( neuronIndex_to, numsyn );
            numneur++;
        }
    }


	// ---
	// --- Sanity Checks
	// ---
	if (numneur != (dims.numneurons))
        error(2,"Bad neural architecture, numneur (",numneur,") not equal to numneurons (",dims.numneurons,")");

    if( numsyn != dims.numsynapses )
	{
		if( (numsyn > dims.numsynapses)
			|| (( (dims.numsynapses - numsyn) / float(dims.numsynapses) ) > 1.e-3) )
		{
			error(2,"Bad neural architecture, numsyn (",numsyn,") not equal to numsynapses (",dims.numsynapses,")");
		}

		dims.numsynapses = numsyn;
	}
	//printf( "numsynapses=%ld\n", numsyn );
	
	// ---
	// --- Calculate Energy Use
	// ---
    energyuse = brain::gNeuralValues.maxneuron2energy * float(dims.numneurons) / float(brain::gNeuralValues.maxneurons)
              + brain::gNeuralValues.maxsynapse2energy * float(dims.numsynapses) / float(brain::gNeuralValues.maxsynapses);

    debugcheck( "after setting up brain architecture" );
}

//---------------------------------------------------------------------------
// Brain::Prebirth
//---------------------------------------------------------------------------
void Brain::Prebirth( long agentNumber, bool recordBrainAnatomy )
{
#if DesignerBrains
	return;
#endif

	// "incept" brain anatomy is as initially generated, with random weights
	if( recordBrainAnatomy )
		dumpAnatomical( "run/brain/anatomy", "incept", agentNumber, 0.0 );
	
    // now send some signals through the system
    // try pure noise for now...
    for( int i = 0; i < brain::gNumPrebirthCycles; i++ )
    {
		cns->prebirthSignal();

		Update( false );
    }
	
	// "birth" anatomy is after gestation (updating with random noise in the inputs for a while)
	if( recordBrainAnatomy )
		dumpAnatomical( "run/brain/anatomy", "birth", agentNumber, 0.0 );

    debugcheck( "after prebirth cycling" );
}

//---------------------------------------------------------------------------
// Brain::Update
//---------------------------------------------------------------------------
void Brain::Update( bool bprint )
{	 
	neuralnet->update( bprint );
}


//---------------------------------------------------------------------------
// Brain::Render
//---------------------------------------------------------------------------
void Brain::Render(short patchwidth, short patchheight)
{
	neuralnet->render( patchwidth, patchheight );
}
