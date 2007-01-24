/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

#define DesignerBrains 0
#define DebugBrainGrow 0

// IntegrateVision, if turned on, will cause the vision neurons to slowly
// integrate what is rendered in front of them into their input neural activation.
// They will do so at a rate defined by TauVision, adjusting color like this:
//     new = TauVision * image  +  (1.0 - TauVision) * old
// IntegrateVision is traditionally off, and TauVision will initially be 0.2.
#define IntegrateVision 0
#define TauVision 0.2

// Self
#include "brain.h"

//#define PRINTBRAIN

#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

// qt
#include <qapplication.h>

// Local
#include "critter.h"
#include "debug.h"
#include "misc.h"
#include "Simulation.h"




// Internal globals
bool brain::classinited;
short* brain::firsteneur;	// [maxneurgroups]
short* brain::firstineur; 	// [maxneurgroups]
float* brain::eeremainder;	// [maxneurgroups]
float* brain::eiremainder;	// [maxneurgroups]
float* brain::iiremainder;	// [maxneurgroups]
float* brain::ieremainder;	// [maxneurgroups]
bool* brain::neurused; 		// [max(maxeneurpergroup,maxineurpergroup)]
short brain::randomneuron;
short brain::energyneuron;
short brain::retinawidth;
short brain::retinaheight;
short brain::gMinWin;

static float initminweight = 0.0; // could read this in

#ifdef PRINTBRAIN
	bool printbrain = true;
	bool brainprinted = false;
#endif PRINTBRAIN

// External globals
NeuralValues brain::gNeuralValues;
long brain::gNumPrebirthCycles;
float brain::gLogisticsSlope;
float brain::gMaxWeight;
float brain::gInitMaxWeight;
float brain::gDecayRate;

enum
{
	kSynapseTypeEE = 0,
	kSynapseTypeIE = 1,
	kSynapseTypeEI = 2,
	kSynapseTypeII = 3
};

#pragma mark -

//===========================================================================
// brain
//===========================================================================

//---------------------------------------------------------------------------
// brain::braininit
//---------------------------------------------------------------------------
void brain::braininit()
{
    if (brain::classinited)
        return;

    brain::classinited = true;

    brain::randomneuron = 0;
    brain::energyneuron = 1;
		
	// remaining neuron-indexes must be determined at "grow" time,
	// now that neural architectures are evolved.
	brain::firsteneur = (short *)calloc(gNeuralValues.maxneurgroups, sizeof(short));
    Q_CHECK_PTR(brain::firsteneur);

	brain::firstineur = (short *)calloc(gNeuralValues.maxneurgroups, sizeof(short));
    Q_CHECK_PTR(brain::firstineur);

	brain::eeremainder = (float *)calloc(gNeuralValues.maxneurgroups, sizeof(float));
    Q_CHECK_PTR(brain::eeremainder);

	brain::eiremainder = (float *)calloc(gNeuralValues.maxneurgroups, sizeof(float));
    Q_CHECK_PTR(brain::eiremainder);

	brain::iiremainder = (float *)calloc(gNeuralValues.maxneurgroups, sizeof(float));
    Q_CHECK_PTR(brain::iiremainder);

	brain::ieremainder = (float *)calloc(gNeuralValues.maxneurgroups, sizeof(float));
    Q_CHECK_PTR(brain::ieremainder);

	brain::neurused = (bool *)calloc(max(gNeuralValues.maxeneurpergroup, gNeuralValues.maxineurpergroup), sizeof(bool));
    Q_CHECK_PTR(brain::neurused);
            
 	// Set up retina values
    brain::retinawidth = max(gMinWin, genome::gMaxvispixels);
    
    if (brain::retinawidth & 1)
        brain::retinawidth++;  // keep it even for efficiency (so I'm told)
        
    brain::retinaheight = gMinWin;
    
    if (brain::retinaheight & 1)
        brain::retinaheight++;        
}


//---------------------------------------------------------------------------
// brain::braindestruct
//---------------------------------------------------------------------------
void brain::braindestruct()
{
	free(brain::firsteneur);
	free(brain::firstineur);
	free(brain::eeremainder);
	free(brain::eiremainder);
	free(brain::iiremainder);
	free(brain::ieremainder);
	free(brain::neurused);
}


//---------------------------------------------------------------------------
// brain::brain
//---------------------------------------------------------------------------
brain::brain()
	:	mygenes(NULL),	// but don't delete them, because we don't new them
		groupblrate(NULL),
		grouplrate(NULL),
		neuron(NULL),
		synapse(NULL),
		neuronactivation(NULL),
		newneuronactivation(NULL),
		neuronsize(0),
		neuronactivationsize(0),
		synapsesize(0)
{
	if (!brain::classinited)
		braininit();	

	retinaBuf = (unsigned char *)calloc(brain::retinawidth * 4, sizeof(unsigned char));
//	cout << "allocated retinaBuf of 4 * width bytes, where width" ses brain::retinawidth nl;
    Q_CHECK_PTR(retinaBuf);
}


//---------------------------------------------------------------------------
// brain::~brain
//---------------------------------------------------------------------------
brain::~brain()
{
	free(neuron);
	free(neuronactivation);
	free(newneuronactivation);
	free(synapse);
	free(groupblrate);
	free(grouplrate);
	free(retinaBuf);
}

//---------------------------------------------------------------------------
// brain::dumpAnatomical
//---------------------------------------------------------------------------
#define DebugDumpAnatomical 0
#if DebugDumpAnatomical
	#define daPrint( x... ) printf( x );
#else
	#define daPrint( x... )
#endif
void brain::dumpAnatomical( char* directoryName, char* suffix, long index, float fitness )
{
	FILE*	file;
	char	filename[256];
	size_t	sizeCM;
	float*	connectionMatrix;
	short	i,j;
	float	maxWeight = max( gMaxWeight, gNeuralValues.maxbias );
	double	inverseMaxWeight = 1. / maxWeight;

	sizeCM = sizeof( *connectionMatrix ) * (numneurons+1) * (numneurons+1);	// +1 for bias neuron
	connectionMatrix = (float*) malloc( sizeCM );
	if( !connectionMatrix )
	{
		fprintf( stderr, "%s: unable to malloc connectionMatrix\n", __FUNCTION__ );
		return;
	}

	bzero( connectionMatrix, sizeCM );

	daPrint( "%s: before filling connectionMatrix\n", __FUNCTION__ );

	// compute the connection matrix
	// columns correspond to presynaptic "from-neurons"
	// rows correspond to postsynaptic "to-neurons"
	long imin = 10000;
	long imax = -10000;
	for( i = 0; i < numsynapses; i++ )
	{
		int cmIndex;
		cmIndex = abs(synapse[i].fromneuron)  +  abs(synapse[i].toneuron) * (numneurons + 1);	// +1 for bias neuron
		if( cmIndex < 0 )
		{
			fprintf( stderr, "cmIndex = %d, i = %d, fromneuron = %d, toneuron = %d, numneurons = %d\n", cmIndex, i, synapse[i].fromneuron, synapse[i].toneuron, numneurons );
		}
		daPrint( "  i=%d, fromneuron=%d, toneuron=%d, cmIndex=%d\n", i, synapse[i].fromneuron, synapse[i].toneuron, cmIndex );
		connectionMatrix[cmIndex] += synapse[i].efficacy;	// the += is so parallel excitatory and inhibitory connections from input and output neurons just sum together
		if( cmIndex < imin )
			imin = cmIndex;
		if( cmIndex > imax )
			imax = cmIndex;
	}
	
	// fill in the biases
	for( i = 0; i < numneurons; i++ )
	{
		int cmIndex = numneurons  +  i * (numneurons + 1);
		connectionMatrix[cmIndex] = neuron[i].bias;
		if( cmIndex < imin )
			imin = cmIndex;
		if( cmIndex > imax )
			imax = cmIndex;
	}

	if( imin < 0 )
		fprintf( stderr, "%s: cmIndex < 0 (%ld)\n", __FUNCTION__, imin );
	if( imax > (numneurons+1)*(numneurons+1) )
		fprintf( stderr, "%s: cmIndex > (numneurons+1)^2 (%ld > %d)\n", __FUNCTION__, imax, (numneurons+1)*(numneurons+1) );

	daPrint( "%s: imin = %ld, imax = %ld, numneurons = %d (+1 for bias)\n", __FUNCTION__, imin, imax, numneurons );

	sprintf( filename, "%s/brainAnatomy_%ld_%s.txt", directoryName, index, suffix );
	file = fopen( filename, "w" );
	if( !file )
	{
		fprintf( stderr, "%s: could not open file %s\n", __FUNCTION__, filename );
		goto bail;
	}

	daPrint( "%s: file = %08lx, index = %ld, fitness = %g\n", __FUNCTION__, (char*)file, index, fitness );

	// print the header, with index, fitness, and number of neurons
	fprintf( file, "brain %ld fitness=%g numneurons+1=%d maxWeight=%g maxBias=%g\n", index, fitness, numneurons+1, gMaxWeight, gNeuralValues.maxbias );

	// print the network architecture
	for( i = 0; i <= numneurons; i++ )	// running over post-synaptic neurons + bias ('=' because of bias)
	{
		for( j = 0; j <= numneurons; j++ )	// running over pre-synaptic neurons + bias ('=' because of bias)
			fprintf( file, "%+06.4f ", connectionMatrix[j + i*(numneurons+1)] * inverseMaxWeight );
		fprintf( file, ";\n" );
	}
	
	fclose( file );

	daPrint( "%s: done with anatomy file for %ld\n", __FUNCTION__, index );

bail:

	daPrint( "%s: about to free connectionMatrix = %08lx\n", __FUNCTION__, (char*)connectionMatrix );

	free( connectionMatrix );
}

//---------------------------------------------------------------------------
// brain::startFunctional
//---------------------------------------------------------------------------
FILE* brain::startFunctional( long index )
{
	FILE* file;
	char filename[256];

	sprintf( filename, "run/brain/function/incomplete_brainFunction_%ld.txt", index );
	file = fopen( filename, "w" );
	if( !file )
	{
		fprintf( stderr, "%s: could not open file %s\n", __FUNCTION__, filename );
		goto bail;
	}

	// print the header, with index (critter number), neuron count, number of input neurons, and number of synapses
	fprintf( file, "brainFunction %ld %d %d %ld\n", index, numneurons, numinputneurons, numsynapses );

bail:

	return( file );
}

//---------------------------------------------------------------------------
// brain::endFunctional
//---------------------------------------------------------------------------
void brain::endFunctional( FILE* file, float fitness )
{
	if( !file )
		return;

	fprintf( file, "end fitness = %g\n", fitness );
	fclose( file );
}

//---------------------------------------------------------------------------
// brain::writeFunctional
//---------------------------------------------------------------------------
void brain::writeFunctional( FILE* file )
{
	if( !file )
		return;

	short i;
	for( i = 0; i < numneurons; i++ )
	{
		fprintf( file, "%d %g\n", i, neuronactivation[i] );
	}
}

//---------------------------------------------------------------------------
// brain::Dump
//---------------------------------------------------------------------------
void brain::Dump(std::ostream& out)
{
    long i;

    out << numneurons sp numsynapses sp numinputneurons sp numnoninputneurons sp energyuse nl;
    out << redneuron sp greenneuron sp blueneuron
		   sp eatneuron sp mateneuron sp fightneuron sp speedneuron
           sp yawneuron sp lightneuron sp focusneuron nl;
    out << numneurgroups sp fNumRedNeurons sp fNumGreenNeurons sp fNumBlueNeurons sp firstnoninputneuron nl;
    out << xredwidth sp xgreenwidth sp xbluewidth sp xredintwidth sp xgreenintwidth sp xblueintwidth nl;

	Q_ASSERT(neuron != NULL);	        
    for (i = 0; i < numneurons; i++)
        out << neuron[i].group sp neuron[i].bias sp neuron[i].startsynapses sp neuron[i].endsynapses nl;

	Q_ASSERT(neuronactivation != NULL);	        
    for (i = 0; i < numneurons; i++)
        out << neuronactivation[i] nl;

	Q_ASSERT(synapse != NULL);	        
    for (i = 0; i < numsynapses; i++)
        out << synapse[i].efficacy
            sp synapse[i].fromneuron sp synapse[i].toneuron nl;

	Q_ASSERT(groupblrate != NULL);	        
    for (i = 0; i < numneurgroups; i++)
        out << groupblrate[i] nl;

	Q_ASSERT(grouplrate != NULL);	        
    for (i = 0; i < (numneurgroups * numneurgroups * 4); i++)
        out << grouplrate[i] nl;

}


//---------------------------------------------------------------------------
// brain::Report
//---------------------------------------------------------------------------
void brain::Report()
{
    cout << numneurons sp numsynapses sp numinputneurons sp numnoninputneurons
        sp energyuse nl;
    cout << redneuron sp greenneuron sp blueneuron
        sp eatneuron sp mateneuron sp fightneuron sp speedneuron
        sp yawneuron sp lightneuron sp focusneuron nl;
    cout << numneurgroups sp fNumRedNeurons sp fNumGreenNeurons sp fNumBlueNeurons
        sp firstnoninputneuron nl;
    cout << xredwidth sp xgreenwidth sp xbluewidth
        sp xredintwidth sp xgreenintwidth sp xblueintwidth nl;
    cout.flush();
}


//---------------------------------------------------------------------------
// brain::Load
//---------------------------------------------------------------------------
void brain::Load(std::istream& in)
{
    long i;

    in >> numneurons >> numsynapses >> numinputneurons >> numnoninputneurons
       >> energyuse;
    in >> redneuron >> greenneuron >> blueneuron
       >> eatneuron >> mateneuron >> fightneuron >> speedneuron
       >> yawneuron >> lightneuron >> focusneuron;
    in >> numneurgroups >> fNumRedNeurons >> fNumGreenNeurons >> fNumBlueNeurons
       >> firstnoninputneuron;
    in >> xredwidth >> xgreenwidth >> xbluewidth
       >> xredintwidth >> xgreenintwidth >> xblueintwidth;

    AllocateBrainMemory(); // if needed

    for (i = 0; i < numneurons; i++)
        in >> neuron[i].group >> neuron[i].bias
           >> neuron[i].startsynapses >> neuron[i].endsynapses;

    for (i = 0; i < numneurons; i++)
        in >> neuronactivation[i];

    for (i = 0; i < numsynapses; i++)
        in >> synapse[i].efficacy 
           >> synapse[i].fromneuron >> synapse[i].toneuron;

    for (i = 0; i < numneurgroups; i++)
        in >> groupblrate[i];

    for (i = 0; i < (numneurgroups*numneurgroups*4); i++)
        in >> grouplrate[i];

}


//---------------------------------------------------------------------------
// brain::AllocateBrainMemory
//---------------------------------------------------------------------------
void brain::AllocateBrainMemory()
{
	// sacrifices speed for minimum memory use
	free(neuron);
	free(neuronactivation);
	free(newneuronactivation);
	free(synapse);
	free(groupblrate);
	free(grouplrate);

    neuron = (neuronstruct *)calloc(numneurons, sizeof(neuronstruct));
    Q_CHECK_PTR(neuron);

	neuronactivation = (float *)calloc(numneurons, sizeof(float));
    Q_CHECK_PTR(neuronactivation);

	newneuronactivation = (float *)calloc(numneurons, sizeof(float));
	Q_CHECK_PTR(newneuronactivation);

	synapse = (synapsestruct *)calloc(numsynapses, sizeof(synapsestruct));
    Q_CHECK_PTR(synapse);

	groupblrate = (float *)calloc(numneurgroups, sizeof(float));
	Q_CHECK_PTR(groupblrate);
	//for (long i = 0; i < numneurgroups; i++)
	//groupblrate[i] = 0.0;

	grouplrate = (float *)calloc(numneurgroups * numneurgroups * 4, sizeof(float));
	Q_CHECK_PTR(grouplrate);        
	//for (int i = 0; i < (numneurgroups*numneurgroups*4); i++)
	//grouplrate[i] = 0.0;
}


//---------------------------------------------------------------------------
// brain::NearestFreeNeuron
//---------------------------------------------------------------------------
short brain::NearestFreeNeuron(short iin, bool* used, short num, short exclude)
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
// brain::GrowDesignedBrain
//---------------------------------------------------------------------------
void brain::GrowDesignedBrain( genome* g )
{
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
	// The seven behaviors, in order, are:  eat, mate, fight, speed (move), yaw (turn), light, focus

	int numDesignExcNeurons[12] = { 1, 1, NumColorNeurons, NumColorNeurons, NumColorNeurons, 1, 1, 1, 1, 1, 1, 1 };
	int numDesignInhNeurons[12] = { 1, 1, NumColorNeurons, NumColorNeurons, NumColorNeurons, 1, 1, 1, 1, 1, 1, 1 };

	mygenes = g;	// not that it matters
	
	// we only have input and output neural groups in this simplifed brain
	numneurgroups = brain::gNeuralValues.numinputneurgroups + brain::gNeuralValues.numoutneurgroups;

#if DebugBrainGrow
    cout << "****************************************************" nlf;
    cout << "Starting a new brain with numneurgroups = " << numneurgroups nlf;
#endif

	numinputneurons = 0;
	short i;
	// For input neural groups, neurons can be both excitatory and inhibitory,
	// but we only count them once
	for( i = 0; i < brain::gNeuralValues.numinputneurgroups; i++ )
	{
		firsteneur[i] = numinputneurons;
		firstineur[i] = numinputneurons;
		numinputneurons += numDesignExcNeurons[i];
		if( i > 4 )	// oops!  just a sanity check
		{
			printf( "%s: ERROR initializing invalid input neural group (%d)\n", __FUNCTION__, i );
			exit( 1 );
		}

	#if DebugBrainGrow
        cout << "group " << i << " has " << numDesignExcNeurons[i] << " neurons" nlf;
	#endif
	}
	firstnoninputneuron = numinputneurons;
	
    // note, group 0 = randomneuron, group 1 = energyneuron
    // group 2 = redneuron(s)
	redneuron = energyneuron + 1;
	fNumRedNeurons = short(numDesignExcNeurons[2]);
	
    // group 3 = greenneuron(s)
    greenneuron = redneuron + fNumRedNeurons;
    fNumGreenNeurons = short(numDesignExcNeurons[3]);
    
    // group 4 = blueneuron(s)
    blueneuron = greenneuron + fNumGreenNeurons;
    fNumBlueNeurons = short(numDesignExcNeurons[4]);

    xredwidth = float(brain::retinawidth) / float(fNumRedNeurons);
    xgreenwidth = float(brain::retinawidth) / float(fNumGreenNeurons);
    xbluewidth = float(brain::retinawidth) / float(fNumBlueNeurons);
    
    xredintwidth = brain::retinawidth / fNumRedNeurons;    
    if ((xredintwidth * fNumRedNeurons) != brain::retinawidth)
        xredintwidth = 0;
        
    xgreenintwidth = brain::retinawidth / fNumGreenNeurons;
    if ((xgreenintwidth*fNumGreenNeurons) != brain::retinawidth)
        xgreenintwidth = 0;
        
    xblueintwidth = brain::retinawidth / fNumBlueNeurons;
    if ((xblueintwidth*fNumBlueNeurons) != brain::retinawidth)
        xblueintwidth = 0;

#if DebugBrainGrow
    cout << "fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = "
         << fNumRedNeurons cms fNumGreenNeurons cms fNumBlueNeurons nlf;;
#endif

	numsynapses = 0;
	numnoninputneurons = 0;
	
	short j, ii;
	for(i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)
	{
		// For this simplified, test brain, there are no purely internal neural groups,
		// so all of these will be output groups, which always consist of a single neuron,
		// that can play the role of both an excitatory and an inhibitory neuron.
		firsteneur[i] = numinputneurons + numnoninputneurons;
		firstineur[i] = numinputneurons + numnoninputneurons;
		numnoninputneurons++;
		
#if DebugBrainGrow
        cout << "group " << i << " has " << numDesignExcNeurons[i] << " e-neurons" nlf;
        cout << "  and " << i << " has " << numDesignInhNeurons[i] << " i-neurons" nlf;
#endif

		// Since we are only dealing with output groups, there is only one neuron in each group,
		// so the synapse count is just the number of neurons providing input to this group.
		// But since the presynaptic groups are all input groups, we're going to let them have
		// both excitatory and inhibitory connections, and then we'll set the synaptic values
		// to do the right thing.  (So count connections from the incoming neurons twice.)
		for( j = 0; j < brain::gNeuralValues.numinputneurgroups; j++ )
		{
			numsynapses += numDesignExcNeurons[j];
			numsynapses += numDesignInhNeurons[j];	// see comment above
			if( j > 4 )	// oops!  just a sanity check
			{
				printf( "%s: ERROR initializing numsynapses, invalid pre-synaptic group (%d)\n", __FUNCTION__, j );
				exit( 1 );
			}
#if DebugBrainGrow
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
#endif
		}
	}
	
    numneurons = numnoninputneurons + numinputneurons;
    if (numneurons > brain::gNeuralValues.maxneurons)
        error(2,"numneurons (",numneurons,") > maxneurons (", brain::gNeuralValues.maxneurons,") in brain::grow");
        
    if (numsynapses > brain::gNeuralValues.maxsynapses)
        error(2,"numsynapses (",numsynapses,") > maxsynapses (", brain::gNeuralValues.maxsynapses,") in brain::grow");

	// set up the ouput/behavior neurons after the input neurons
    eatneuron = numinputneurons;
    mateneuron = eatneuron + 1;
    fightneuron = mateneuron + 1;
    speedneuron = fightneuron + 1;
    yawneuron = speedneuron + 1;
    lightneuron = yawneuron + 1;
    focusneuron = lightneuron + 1;
	
	numOutputNeurons = 7;
	firstOutputNeuron = eatneuron;
	firstInternalNeuron = eatneuron + numOutputNeurons;

	AllocateBrainMemory();
	
    short ineur, jneur, nneuri, nneurj, joff, disneur;
    short isyn, newsyn;
    long  nsynij;
    float nsynijperneur;
    long numsyn = 0;
    short numneur = numinputneurons;
    float tdij;

    for (i = 0, ineur = 0; i < brain::gNeuralValues.numinputneurgroups; i++)		//for all input neural groups
    {
        for (j = 0; j < numDesignExcNeurons[i]; j++, ineur++)
        {
            neuron[ineur].group = i;
            neuron[ineur].bias = 0.0;         // not used
            neuron[ineur].startsynapses = -1; // not used
            neuron[ineur].endsynapses = -1;   // not used
        }
    }

    for (i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)		//for all behavior (and the non-existent internal) neural groups
    {
#if DebugBrainGrow
        cout << "For group " << i << ":" nlf;
#endif

        float groupbias;
		
		if( i == brain::gNeuralValues.numinputneurgroups + 4 )	// yaw group/neuron		
			groupbias = 0.5;	// keep turning, unless overriden by vision
		else
			groupbias = 0.0;	// no bias	// g->Bias(i);
        groupblrate[i] = 0.0;	// no bias learning	// g->BiasLearningRate(i);

#if DebugBrainGrow
        cout << "  groupbias = " << groupbias nlf;
        cout << "  groupbiaslearningrate = " << groupblrate[i] nlf;
#endif

        for (j = 0; j < numneurgroups; j++)
        {
            eeremainder[j] = 0.0;
            eiremainder[j] = 0.0;
            iiremainder[j] = 0.0;
            ieremainder[j] = 0.0;

            grouplrate[index4(i, j, 0, 0, numneurgroups, 2, 2)] = 0.000;	// no learning	// very slow, uniform learning rate, for now
            grouplrate[index4(i, j, 0, 1, numneurgroups, 2, 2)] = 0.000;	// no learning	// very slow, uniform learning rate, for now
            grouplrate[index4(i, j, 1, 1, numneurgroups, 2, 2)] = 0.000;	// no learning	// very slow, uniform learning rate, for now
            grouplrate[index4(i, j, 1, 0, numneurgroups, 2, 2)] = 0.000;	// no learning	// very slow, uniform learning rate, for now
        }

        // setup all e-neurons for this group
        nneuri = numDesignExcNeurons[i];

#if DebugBrainGrow
        cout << "  Setting up " << nneuri << " e-neurons" nlf;
#endif
		
		short ini;	
        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firsteneur[i];

#if DebugBrainGrow
            cout << "  For ini, ineur = "
                 << ini cms ineur << ":" nlf;
#endif

            neuron[ineur].group = i;
            neuron[ineur].bias = groupbias;
            neuron[ineur].startsynapses = numsyn;

#if DebugBrainGrow
            cout << "    group = " << neuron[ineur].group nlf;
            cout << "    bias = " << neuron[ineur].bias nlf;
            cout << "    startsynapses = " << neuron[ineur].startsynapses nlf;
            cout << "    Setting up e-e connections:" nlf;
#endif

            // setup all e-e connections for this e-neuron
            for (j = 0; j < numneurgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynij = 0;
				}
				else
				{
					nneurj = numDesignExcNeurons[j];	// g->numeneur(j);
					nsynij = numDesignExcNeurons[j];	// g->numeesynapses(i,j);
				}

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)eeremainder = "
                     << nneurj cms eeremainder[j] nlf;
#endif

                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + eeremainder[j] + 1.e-5);
                eeremainder[j] += nsynijperneur - newsyn;
                tdij = 0.0;	// no topological distortion // g->eetd(i,j);

                joff = short((float(ini) / float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)eeremainder, tdij, joff = "
                     << eeremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur,&neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firsteneur[j];

                    synapse[numsyn].fromneuron =  jneur; // + denotes excitatory
                    synapse[numsyn].toneuron   =  ineur; // + denotes excitatory
                    
                    if (ineur == jneur)
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = DesignedEfficacy( i, j, isyn, kSynapseTypeEE );	// reset this later for the designed brain

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            // setup all i-e connections for this e-neuron

#if DebugBrainGrow
            cout << "    Setting up i-e connections:" nlf;
#endif

            for (j = 0; j < numneurgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynij = 0;
				}
				else
				{
					nneurj = numDesignInhNeurons[j];	// g->numineur(j);
					nsynij = numDesignInhNeurons[j];	// g->numiesynapses(i,j);
				}

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)ieremainder = "
                     << nneurj cms ieremainder[j] nlf;
#endif

                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + ieremainder[j] + 1.e-5);
                ieremainder[j] += nsynijperneur - newsyn;
                tdij = 0.0;	// no topological distortion	// g->ietd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj)
                     - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)ieremainder, tdij, joff = "
                     << ieremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur,&neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firstineur[j];

                    synapse[numsyn].fromneuron = -jneur; // - denotes inhibitory
                    synapse[numsyn].toneuron   =  ineur; // + denotes excitatory
                    
                    if (ineur == jneur) // can't happen anymore?
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = min(-1.e-10, (double) DesignedEfficacy( i, j, isyn, kSynapseTypeIE ));

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            neuron[ineur].endsynapses = numsyn;
            numneur++;
        }

        // setup all i-neurons for this group

        if( IsOutputNeuralGroup( i ) )
            nneuri = 0;  // output/behavior neurons are e-only postsynaptically
        else
            nneuri = numDesignInhNeurons[i];	// g->numineur(i);

#if DebugBrainGrow
        cout << "  Setting up " << nneuri << " i-neurons" nlf;
#endif

        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firstineur[i];

#if DebugBrainGrow
            cout << "  For ini, ineur = "
                 << ini cms ineur << ":" nlf;
#endif

            neuron[ineur].group = i;
            neuron[ineur].bias = groupbias;
            neuron[ineur].startsynapses = numsyn;

#if DebugBrainGrow
            cout << "    group = " << neuron[ineur].group nlf;
            cout << "    bias = " << neuron[ineur].bias nlf;
            cout << "    startsynapses = " << neuron[ineur].startsynapses nlf;
            cout << "    Setting up e-i connections:" nlf;
#endif

            // setup all e-i connections for this i-neuron

            for (j = 0; j < numneurgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynij = 0;
				}
				else
				{
					nneurj = numDesignExcNeurons[j];	// g->numeneur(j);
					nsynij = numDesignExcNeurons[j];	// g->numeisynapses(i, j);
				}

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)eiremainder = "
                     << nneurj cms eiremainder[j] nlf;
#endif

                nsynijperneur = float(nsynij) / float(nneuri);
                newsyn = short(nsynijperneur + eiremainder[j] + 1.e-5);
                eiremainder[j] += nsynijperneur - newsyn;
                tdij = 0.0;	// no topological distortion	// g->eitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)eiremainder, tdij, joff = "
                     << eiremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ini);
                        else
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firsteneur[j];

                    synapse[numsyn].fromneuron =  jneur; // + denotes excitatory
                    synapse[numsyn].toneuron   = -ineur; // - denotes inhibitory
                    
                    if (ineur == jneur) // can't happen anymore?
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = DesignedEfficacy( i, j, isyn, kSynapseTypeEI );

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            // setup all i-i connections for this i-neuron
            for (j = 0; j < numneurgroups; j++)
            {
				// all i-groups are output groups, so if the j-group is an output group,
				// there should be no connection
				if( IsOutputNeuralGroup( j ) )	// j is an output group
				{
					nneurj = 0;
					nsynij = 0;
				}
				else
				{
					nneurj = numDesignInhNeurons[j];	// g->numineur(j);
					nsynij = numDesignInhNeurons[j];	// g->numiisynapses(i,j);
				}

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)iiremainder = "
                     << nneurj cms iiremainder[j] nlf;
#endif

                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + iiremainder[j] + 1.e-5);
                iiremainder[j] += nsynijperneur - newsyn;
                tdij = 0.0;	// no topological distortion	// g->iitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)iiremainder, tdij, joff = "
                     << iiremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ini);
                        else
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firstineur[j];

                    synapse[numsyn].fromneuron = -jneur; // - denotes inhibitory
                    synapse[numsyn].toneuron   = -ineur; // - denotes inhibitory
                    
                    if (ineur == jneur) // can't happen anymore?
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = min(-1.e-10, (double) DesignedEfficacy( i, j, isyn, kSynapseTypeII ));

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            neuron[ineur].endsynapses = numsyn;
            numneur++;
        }
    }

    if (numneur != (numneurons))
        error(2,"Bad neural architecture, numneur (",numneur,") not equal to numneurons (",numneurons,")");

    if (numsyn != (numsynapses))
        error(2,"Bad neural architecture, numsyn (",numsyn,") not equal to numsynapses (",numsynapses,")");

    for (i = 0; i < numneurons; i++)
        neuronactivation[i] = 0.0;	// 0.1	// 0.5;

    energyuse = brain::gNeuralValues.maxneuron2energy * float(numneurons) / float(brain::gNeuralValues.maxneurons)
              + brain::gNeuralValues.maxsynapse2energy * float(numsynapses) / float(brain::gNeuralValues.maxsynapses);

#ifdef DEBUGCHECK
    debugcheck("brain::grow after setting up architecture");
#endif DEBUGCHECK

#if 0
    // now send some signals through the system
    // try pure noise for now...
    for (i = 0; i < gNumPrebirthCycles; i++)
    {
        // load up the retinabuf with noise
        for (j = 0; j < (brain::retinawidth * 4); j++)
            retinaBuf[j] = (unsigned char)(rrand(0.0, 255.0));
        Update(drand48());
    }
#endif
}


//---------------------------------------------------------------------------
// brain::DesignedEfficacy
//---------------------------------------------------------------------------
#define DebugDesignedEfficacy 0
#if DebugDesignedEfficacy
	#define dePrint( x... ) printf( x )
#else
	#define dePrint( x... )
#endif
float brain::DesignedEfficacy( short toGroup, short fromGroup, short isyn, int synapseType )
{
	float efficacy = 0.0;	// may be set < 0.0 for inhibitory connections in caller
	
	switch( toGroup )
	{
		case 5:	// eat
			if( (fromGroup == 3) && (synapseType == kSynapseTypeEE) )	// green vision
			{
				dePrint( "%s: setting efficacy to %g for green vision connecting to the eat behavior\n", __FUNCTION__, gMaxWeight );
				efficacy = gMaxWeight;
			}
			break;
		
		case 6:	// mate
			if( (fromGroup == 4) && (synapseType == kSynapseTypeEE) )	// blue vision
			{
				dePrint( "%s: setting efficacy to %g for blue vision connecting to the mate behavior\n", __FUNCTION__, gMaxWeight );
				efficacy = gMaxWeight;
			}
			break;
		
		case 7:	// fight
			if( (fromGroup == 2) && (synapseType == kSynapseTypeEE) )	// red vision
			{
				dePrint( "%s: setting efficacy to %g for red vision connecting to the fight behavior\n", __FUNCTION__, gMaxWeight );
				efficacy = gMaxWeight;
			}
			break;
		
		case 8:	// speed (move)
			if( (fromGroup > 1) && (fromGroup < 5) && (synapseType == kSynapseTypeEE) )	// any vision
			{
				dePrint( "%s: setting efficacy to %g for any vision connecting to the move behavior\n", __FUNCTION__, gMaxWeight );
				efficacy = gMaxWeight;
			}
			break;
		
		case 9:	// yaw (turn)
			switch( fromGroup )
			{
				case 2:	// red vision
					if( (isyn < LeftColorNeurons) && ((synapseType == kSynapseTypeIE) || (synapseType == kSynapseTypeII)) )
					{
						dePrint( "%s: setting efficacy to %g for left red vision connecting to the turn behavior\n", __FUNCTION__, -gMaxWeight );
						efficacy = -gMaxWeight;	// input on left forces turn to right (negative yaw)
					}
					else if( (isyn >= LeftColorNeurons) && ((synapseType == kSynapseTypeEE) || (synapseType == kSynapseTypeEI)) )
					{
						dePrint( "%s: setting efficacy to %g for right red vision connecting to the turn behavior\n", __FUNCTION__, gMaxWeight );
						efficacy = gMaxWeight;		// input on right forces turn to left (positive yaw)
					}
					break;
					
				case 3:	// green vision
					if( (isyn < LeftColorNeurons) && ((synapseType == kSynapseTypeEE) || (synapseType == kSynapseTypeEI)) )
					{
						dePrint( "%s: setting efficacy to %g for left green vision connecting to the turn behavior\n", __FUNCTION__, gMaxWeight * 0.5 );
						efficacy = gMaxWeight * 0.5;	// input on left forces turn to left (positive yaw)
					}
					else if( (isyn >= LeftColorNeurons) && ((synapseType == kSynapseTypeIE) || (synapseType == kSynapseTypeII)) )
					{
						dePrint( "%s: setting efficacy to %g for right green vision connecting to the turn behavior\n", __FUNCTION__, -gMaxWeight * 0.5 );
						efficacy = -gMaxWeight * 0.5;		// input on right green forces turn to right (negative yaw)
					}
					break;
				
				case 4:	// blue vision
					if( (isyn < LeftColorNeurons) && ((synapseType == kSynapseTypeEE) || (synapseType == kSynapseTypeEI)) )
					{
						dePrint( "%s: setting efficacy to %g for left blue vision connecting to the turn behavior\n", __FUNCTION__, gMaxWeight * 0.75 );
						efficacy = gMaxWeight * 0.75;	// input on left forces turn to left (positive yaw)
					}
					else if( (isyn >= LeftColorNeurons) && ((synapseType == kSynapseTypeIE) || (synapseType == kSynapseTypeII)) )
					{
						dePrint( "%s: setting efficacy to %g for right blue vision connecting to the turn behavior\n", __FUNCTION__, -gMaxWeight * 0.75 );
						efficacy = -gMaxWeight * 0.75;		// input on right blue forces turn to right (negative yaw)
					}
					break;
				
				default:	// ignore other inputs
					break;
			}
			break;
		
		case 10:	// light
			break;
		
		case 11:	// focus
			break;
		
		default:
			printf( "%s: invalid toGroup (%d)\n", __FUNCTION__, toGroup );
			break;
	}
	
	return( efficacy );
}


//---------------------------------------------------------------------------
// brain::Grow
//---------------------------------------------------------------------------
void brain::Grow( genome* g, long critterNumber, bool recordBrainAnatomy )
{
#ifdef DEBUGCHECK
    debugcheck("brain::grow entry");
#endif DEBUGCHECK

#if DesignerBrains
	GrowDesignedBrain( g );
	return;
#endif

    mygenes = g;

    numneurgroups = g->NumNeuronGroups();

#if DebugBrainGrow
    cout << "****************************************************" nlf;
    cout << "Starting a new brain with numneurgroups = " << numneurgroups nlf;
#endif
    
	// First establish the starting neuron indexes for the input neuron groups
    short i;
    numinputneurons = 0;
    for (i = 0; i < brain::gNeuralValues.numinputneurgroups; i++)
    {
        firsteneur[i] = numinputneurons;
        firstineur[i] = numinputneurons; // input neurons double as e & i
        numinputneurons += g->numeneur(i);

#if DebugBrainGrow
        cout << "group " << i << " has " << g->numeneur(i) << " neurons" nlf;
#endif

    }
    firstnoninputneuron = numinputneurons;

#if DebugBrainGrow
    cout << "numinputneurons = " << numinputneurons nlf;
#endif

    // note, group 0 = randomneuron, group 1 = energyneuron
    // group 2 = redneuron(s)
    redneuron = energyneuron + 1;
    fNumRedNeurons = short(g->numeneur(2));
    
    // group 3 = greenneuron(s)
    greenneuron = redneuron + fNumRedNeurons;
    fNumGreenNeurons = short(g->numeneur(3));
    
    // group 4 = blueneuron(s)
    blueneuron = greenneuron + fNumGreenNeurons;
    fNumBlueNeurons = short(g->numeneur(4));

    xredwidth = float(brain::retinawidth) / float(fNumRedNeurons);
    xgreenwidth = float(brain::retinawidth) / float(fNumGreenNeurons);
    xbluewidth = float(brain::retinawidth) / float(fNumBlueNeurons);
    
    xredintwidth = brain::retinawidth / fNumRedNeurons;    
    if ((xredintwidth * fNumRedNeurons) != brain::retinawidth)
        xredintwidth = 0;
        
    xgreenintwidth = brain::retinawidth / fNumGreenNeurons;
    if ((xgreenintwidth*fNumGreenNeurons) != brain::retinawidth)
        xgreenintwidth = 0;
        
    xblueintwidth = brain::retinawidth / fNumBlueNeurons;
    if ((xblueintwidth*fNumBlueNeurons) != brain::retinawidth)
        xblueintwidth = 0;

#if DebugBrainGrow
    cout << "fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = "
         << fNumRedNeurons cms fNumGreenNeurons cms fNumBlueNeurons nlf;;
#endif

    numsynapses = 0;
    numnoninputneurons = 0;
    
    short j, ii;
    for( i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++ )
    {
        firsteneur[i] = numinputneurons + numnoninputneurons;

		// Since output groups are both e & i, we have to be careful and only count
		// one or the other, but for internal groups we count both
		if( IsInternalNeuralGroup( i ) )
            numnoninputneurons += g->numeneur(i);

        firstineur[i] = numinputneurons + numnoninputneurons;
        numnoninputneurons += g->numineur(i);

#if DebugBrainGrow
        cout << "group " << i << " has " << g->numeneur(i) << " e-neurons" nlf;
        cout << "  and " << i << " has " << g->numineur(i) << " i-neurons" nlf;
#endif

        for (j = 0; j < numneurgroups; j++)
        {
            numsynapses += g->numsynapses(i,j);

#if DebugBrainGrow
            cout << "  from " << j << " to " << i << " there are "
                 << g->numeesynapses(i,j) << " e-e synapses" nlf;
            cout << "  from " << j << " to " << i << " there are "
                 << g->numiesynapses(i,j) << " i-e synapses" nlf;
            cout << "  from " << j << " to " << i << " there are "
                 << g->numeisynapses(i,j) << " e-i synapses" nlf;
            cout << "  from " << j << " to " << i << " there are "
                 << g->numiisynapses(i,j) << " i-i synapses" nlf;
            cout << "  from " << j << " to " << i << " there are "
                 << g->numsynapses(i,j) << " total synapses" nlf;
#endif

        }
    }
    
    numneurons = numnoninputneurons + numinputneurons;
	if( numneurons > brain::gNeuralValues.maxneurons )
		error( 2, "numneurons (", numneurons, ") > maxneurons (", brain::gNeuralValues.maxneurons, ") in brain::grow" );
        
	if( numsynapses > brain::gNeuralValues.maxsynapses )
		error( 2, "numsynapses (", numsynapses, ") > maxsynapses (", brain::gNeuralValues.maxsynapses, ") in brain::grow" );

	// set up the ouput/behavior neurons after the input neurons
    eatneuron = numinputneurons;
    mateneuron = eatneuron + 1;
    fightneuron = mateneuron + 1;
    speedneuron = fightneuron + 1;
    yawneuron = speedneuron + 1;
    lightneuron = yawneuron + 1;
    focusneuron = lightneuron + 1;

	numOutputNeurons = 7;
	firstOutputNeuron = eatneuron;
	firstInternalNeuron = eatneuron + numOutputNeurons;

#if DebugBrainGrow
    cout << "numneurons = " << numneurons << "  (of " << brain::gNeuralValues.maxneurons pnlf;
    cout << "numsynapses = " << numsynapses << "  (of " << brain::gNeuralValues.maxsynapses pnlf;
#endif

#ifdef DEBUGCHECK
    debugcheck("brain::grow before allocating memory");
#endif DEBUGCHECK

	AllocateBrainMemory(); // if needed

#ifdef DEBUGCHECK
    debugcheck("brain::grow after allocating memory");
#endif DEBUGCHECK

    short ineur, jneur, nneuri, nneurj, joff, disneur;
    short isyn, newsyn;
    long  nsynij;
    float nsynijperneur;
    long numsyn = 0;
    short numneur = numinputneurons;
    float tdij;

    for (i = 0, ineur = 0; i < brain::gNeuralValues.numinputneurgroups; i++)
    {
        for (j = 0; j < g->numeneur(i); j++, ineur++)
        {
            neuron[ineur].group = i;
            neuron[ineur].bias = 0.0;         // not used
            neuron[ineur].startsynapses = -1; // not used
            neuron[ineur].endsynapses = -1;   // not used
        }
    }

    for (i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)
    {
#if DebugBrainGrow
        cout << "For group " << i << ":" nlf;
#endif

        float groupbias = g->Bias(i);
        groupblrate[i] = g->BiasLearningRate(i);

#if DebugBrainGrow
        cout << "  groupbias = " << groupbias nlf;
        cout << "  groupbiaslearningrate = " << groupblrate[i] nlf;
#endif

        for (j = 0; j < numneurgroups; j++)
        {
            eeremainder[j] = 0.0;
            eiremainder[j] = 0.0;
            iiremainder[j] = 0.0;
            ieremainder[j] = 0.0;

            grouplrate[index4(i, j, 0, 0, numneurgroups, 2, 2)] = g->eelr(i, j);
            grouplrate[index4(i, j, 0, 1, numneurgroups, 2, 2)] = g->ielr(i, j);
            grouplrate[index4(i, j, 1, 1, numneurgroups, 2, 2)] = g->iilr(i, j);
            grouplrate[index4(i, j, 1, 0, numneurgroups, 2, 2)] = g->eilr(i, j);
        }

        // setup all e-neurons for this group
        nneuri = g->numeneur(i);

#if DebugBrainGrow
        cout << "  Setting up " << nneuri << " e-neurons" nlf;
#endif
		
		short ini;	
        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firsteneur[i];

#if DebugBrainGrow
            cout << "  For ini, ineur = "
                 << ini cms ineur << ":" nlf;
#endif

            neuron[ineur].group = i;
            neuron[ineur].bias = groupbias;
            neuron[ineur].startsynapses = numsyn;

#if DebugBrainGrow
            cout << "    group = " << neuron[ineur].group nlf;
            cout << "    bias = " << neuron[ineur].bias nlf;
            cout << "    startsynapses = " << neuron[ineur].startsynapses nlf;
            cout << "    Setting up e-e connections:" nlf;
#endif

            // setup all e-e connections for this e-neuron
            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numeneur(j);

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)eeremainder = "
                     << nneurj cms eeremainder[j] nlf;
#endif

                nsynij = g->numeesynapses(i,j);
                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + eeremainder[j] + 1.e-5);
                eeremainder[j] += nsynijperneur - newsyn;
                tdij = g->eetd(i,j);

                joff = short((float(ini) / float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)eeremainder, tdij, joff = "
                     << eeremainder[j] cms tdij cms joff nlf;
#endif

                if ((joff + newsyn) > nneurj)
                {
                    error(2,"Illegal architecture generated: ",
                        "more e-e synapses from group ",j,
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                        if (i == j) // same group and neuron type (because we're doing e->e connections currently)
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur,&neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firsteneur[j];

                    synapse[numsyn].fromneuron =  jneur; // + denotes excitatory
                    synapse[numsyn].toneuron   =  ineur; // + denotes excitatory
                    
                    if (ineur == jneur)
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = rrand(initminweight, gInitMaxWeight);

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            // setup all i-e connections for this e-neuron

#if DebugBrainGrow
            cout << "    Setting up i-e connections:" nlf;
#endif

            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numineur(j);

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)ieremainder = "
                     << nneurj cms ieremainder[j] nlf;
#endif

                nsynij = g->numiesynapses(i,j);
                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + ieremainder[j] + 1.e-5);
                ieremainder[j] += nsynijperneur - newsyn;
                tdij = g->ietd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj)
                     - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)ieremainder, tdij, joff = "
                     << ieremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                        if( (i == j) && IsOutputNeuralGroup( i ) )	// same & output group
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur,&neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firstineur[j];

                    synapse[numsyn].fromneuron = -jneur; // - denotes inhibitory
                    synapse[numsyn].toneuron   =  ineur; // + denotes excitatory
                    
                    if (ineur == jneur) // can't happen anymore?
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = min(-1.e-10, -rrand(initminweight, gInitMaxWeight));

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            neuron[ineur].endsynapses = numsyn;
            numneur++;
        }

        // setup all i-neurons for this group

        if( IsOutputNeuralGroup( i ) )
            nneuri = 0;  // output/behavior neurons are e-only postsynaptically
        else
            nneuri = g->numineur( i );

#if DebugBrainGrow
        cout << "  Setting up " << nneuri << " i-neurons" nlf;
#endif

        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firstineur[i];

#if DebugBrainGrow
            cout << "  For ini, ineur = "
                 << ini cms ineur << ":" nlf;
#endif

            neuron[ineur].group = i;
            neuron[ineur].bias = groupbias;
            neuron[ineur].startsynapses = numsyn;

#if DebugBrainGrow
            cout << "    group = " << neuron[ineur].group nlf;
            cout << "    bias = " << neuron[ineur].bias nlf;
            cout << "    startsynapses = " << neuron[ineur].startsynapses nlf;
            cout << "    Setting up e-i connections:" nlf;
#endif

            // setup all e-i connections for this i-neuron

            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numeneur(j);

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)eiremainder = "
                     << nneurj cms eiremainder[j] nlf;
#endif

                nsynij = g->numeisynapses(i, j);
                nsynijperneur = float(nsynij) / float(nneuri);
                newsyn = short(nsynijperneur + eiremainder[j] + 1.e-5);
                eiremainder[j] += nsynijperneur - newsyn;
                tdij = g->eitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)eiremainder, tdij, joff = "
                     << eiremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                        if( (i == j) && IsOutputNeuralGroup( i ) )	// same & output group
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firsteneur[j];

                    synapse[numsyn].fromneuron =  jneur; // + denotes excitatory
                    synapse[numsyn].toneuron   = -ineur; // - denotes inhibitory
                    
                    if (ineur == jneur) // can't happen anymore?
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = rrand(initminweight, gInitMaxWeight);

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            // setup all i-i connections for this i-neuron
            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numineur(j);

#if DebugBrainGrow
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)iiremainder = "
                     << nneurj cms iiremainder[j] nlf;
#endif

                nsynij = g->numiisynapses(i,j);
                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + iiremainder[j] + 1.e-5);
                iiremainder[j] += nsynijperneur - newsyn;
                tdij = g->iitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#if DebugBrainGrow
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)iiremainder, tdij, joff = "
                     << iiremainder[j] cms tdij cms joff nlf;
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
                    if (drand48() < tdij)
                    {
                        disneur = short(nint(rrand(-0.5,0.5)*tdij*nneurj));
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
                        if( i == j ) // same group and neuron type (because we're doing i->i currently)
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ineur);	// ineur was ini
                        else
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, jneur);
                    }

                    neurused[jneur] = true;

                    jneur += firstineur[j];

                    synapse[numsyn].fromneuron = -jneur; // - denotes inhibitory
                    synapse[numsyn].toneuron   = -ineur; // - denotes inhibitory
                    
                    if (ineur == jneur) // can't happen anymore?
                        synapse[numsyn].efficacy = 0.0;
                    else
                        synapse[numsyn].efficacy = min(-1.e-10, -rrand(initminweight, gInitMaxWeight));

#if DebugBrainGrow
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif

                    numsyn++;
                }
            }

            neuron[ineur].endsynapses = numsyn;
            numneur++;
        }
    }

    if (numneur != (numneurons))
        error(2,"Bad neural architecture, numneur (",numneur,") not equal to numneurons (",numneurons,")");

    if (numsyn != (numsynapses))
        error(2,"Bad neural architecture, numsyn (",numsyn,") not equal to numsynapses (",numsynapses,")");

    for (i = 0; i < numneurons; i++)
        neuronactivation[i] = 0.1;  // 0.5;

    energyuse = brain::gNeuralValues.maxneuron2energy * float(numneurons) / float(brain::gNeuralValues.maxneurons)
              + brain::gNeuralValues.maxsynapse2energy * float(numsynapses) / float(brain::gNeuralValues.maxsynapses);

#ifdef DEBUGCHECK
    debugcheck("brain::grow after setting up architecture");
#endif DEBUGCHECK

	// "incept" brain anatomy is as initially generated, with random weights
	if( recordBrainAnatomy )
		dumpAnatomical( "run/brain/anatomy", "incept", critterNumber, 0.0 );
	
    // now send some signals through the system
    // try pure noise for now...
    for (i = 0; i < gNumPrebirthCycles; i++)
    {
        // load up the retinabuf with noise
        for (j = 0; j < (brain::retinawidth * 4); j++)
            retinaBuf[j] = (unsigned char)(rrand(0.0, 255.0));
        Update(drand48());
    }
	
	// "birth" anatomy is after gestation (updating with random noise in the inputs for a while)
	if( recordBrainAnatomy )
		dumpAnatomical( "run/brain/anatomy", "birth", critterNumber, 0.0 );

#ifdef DEBUGCHECK
    debugcheck("brain::grow after prebirth cycling");
#endif DEBUGCHECK
}


void brain::Update(float energyfraction)
{	 
#ifdef DEBUGCHECK
    debugcheck("brain::Update entry");
#endif DEBUGCHECK

    short i,j,ii,jj;
    long k;
    if ((neuron == NULL) || (synapse == NULL) || (neuronactivation == NULL))
        return;

#ifdef PRINTBRAIN
    if (printbrain && TSimulation::fOverHeadRank && !brainprinted && critter::currentCritter == TSimulation::fMonitorCritter)
    {
        brainprinted = true;
        printf("neuron (toneuron)  fromneuron   synapse   efficacy\n");
        
        for( i = firstnoninputneuron; i < numneurons; i++ )
        {
            for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
            {
				printf("%3d   %3d    %3d    %5ld    %f\n",
					   i, synapse[k].toneuron, synapse[k].fromneuron,
					   k, synapse[k].efficacy); 
            }
        }
        printf("fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = %d, %d, %d\n",
                fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons);
                
        printf("xredwidth, xgreenwidth, xbluewidth = %g, %g, %g\n",
                xredwidth, xgreenwidth, xbluewidth);
                
        printf("xredintwidth, xgreenintwidth, xblueintwidth = %d, %d, %d\n",
                xredintwidth, xgreenintwidth, xblueintwidth);
    }
#endif PRINTBRAIN

    neuronactivation[randomneuron] = drand48();
    neuronactivation[energyneuron] = energyfraction;
    
    short pixel;
    float avgcolor;
    float endpixloc;
    
    if (xredintwidth)
    {
        pixel = 0;
        for (i = 0; i < fNumRedNeurons; i++)
        {
            avgcolor = 0.0;
            for (short ipix = 0; ipix < xredintwidth; ipix++)
                avgcolor += retinaBuf[(pixel++) * 4];
            neuronactivation[redneuron+i] = avgcolor / (xredwidth * 255.0);
        }
    }
    else
    {
        pixel = 0;
        avgcolor = 0.0;
	#ifdef PRINTBRAIN
        if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
        {
            printf("xredwidth = %f\n", xredwidth);
        }
	#endif PRINTBRAIN
        for (i = 0; i < fNumRedNeurons; i++)
        {
            endpixloc = xredwidth * float(i+1);
		#ifdef PRINTBRAIN
            if (printbrain &&
                (critter::currentCritter == TSimulation::fMonitorCritter))
            {
                printf("  neuron %d, endpixloc = %g\n", i, endpixloc);
            }
		#endif PRINTBRAIN
            while (float(pixel) < (endpixloc - 1.0))
            {
                avgcolor += retinaBuf[(pixel++) * 4];
			#ifdef PRINTBRAIN
                if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
                {
                    printf("    in loop with pixel %d, avgcolor = %g\n", pixel,avgcolor);
					if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc - 1.0 - 1.0e-5)))
						printf("Got in-loop borderline case - red\n");
                }
			#endif PRINTBRAIN
            }
            
            avgcolor += (endpixloc - float(pixel)) * retinaBuf[pixel * 4];
            neuronactivation[redneuron + i] = avgcolor / (xredwidth * 255.0);
		#ifdef PRINTBRAIN
            if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
            {
                printf("    after loop with pixel %d, avgcolor = %g, color = %g\n", pixel,avgcolor,neuronactivation[redneuron+i]);
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0 + 1.0e-5)))
                    printf("Got outside-loop borderline case - red\n");
            }
		#endif PRINTBRAIN
            avgcolor = (1.0 - (endpixloc - float(pixel))) * retinaBuf[pixel * 4];
		#ifdef PRINTBRAIN
            if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
            {
                printf("  before incrementing pixel = %d, avgcolor = %g\n", pixel, avgcolor);
            }
		#endif PRINTBRAIN
            pixel++;
        }
    }
    
    if (xgreenintwidth)
    {
        pixel = 0;
        for (i = 0; i < fNumGreenNeurons; i++)
        {
            avgcolor = 0.0;
            for (short ipix = 0; ipix < xgreenintwidth; ipix++)
                avgcolor += retinaBuf[(pixel++) * 4 + 1];
            neuronactivation[greenneuron + i] = avgcolor / (xgreenwidth * 255.0);
        }
    }
    else
    {
        pixel = 0;
        avgcolor = 0.0;
        for (i = 0; i < fNumGreenNeurons; i++)
        {
            endpixloc = xgreenwidth * float(i+1);
            while (float(pixel) < (endpixloc - 1.0))
            {
                avgcolor += retinaBuf[(pixel++) * 4 + 1];
			#ifdef PRINTBRAIN
                if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
                {
                    if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc - 1.0 - 1.0e-5)) )
                        printf("Got in-loop borderline case - green\n");
                }
			#endif PRINTBRAIN
            }
		#ifdef PRINTBRAIN
            if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
            {
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0)))
                    printf("Got outside-loop borderline case - green\n");
            }
		#endif PRINTBRAIN
            avgcolor += (endpixloc - float(pixel)) * retinaBuf[pixel * 4 + 1];
            neuronactivation[greenneuron + i] = avgcolor / (xgreenwidth * 255.0);
            avgcolor = (1.0 - (endpixloc - float(pixel))) * retinaBuf[pixel * 4 + 1];
            pixel++;
        }
    }
    
    if (xblueintwidth)
    {
        pixel = 0;
        for (i = 0; i < fNumBlueNeurons; i++)
        {
            avgcolor = 0.0;
            for (short ipix = 0; ipix < xblueintwidth; ipix++)
                avgcolor += retinaBuf[(pixel++) * 4 + 2];
            neuronactivation[blueneuron+i] = avgcolor / (xbluewidth * 255.0);
        }
    }
    else
    {
        pixel = 0;
        avgcolor = 0.0;
        
        for (i = 0; i < fNumBlueNeurons; i++)
        {
            endpixloc = xbluewidth * float(i + 1);
			while (float(pixel) < (endpixloc - 1.0 /*+ 1.e-5*/))
            {
                avgcolor += retinaBuf[(pixel++) * 4 + 2];
			#ifdef PRINTBRAIN
                if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
                {
                    if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc - 1.0 - 1.0e-5)) )
                        printf("Got in-loop borderline case - blue\n");
                }
			#endif PRINTBRAIN
            }
            
		#ifdef PRINTBRAIN
            if (printbrain && (critter::currentCritter == TSimulation::fMonitorCritter))
            {
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0 + 1.0e-5)) )
                    printf("Got outside-loop borderline case - blue\n");
            }
		#endif PRINTBRAIN

            if (pixel < brain::retinawidth)  // TODO How do we end up overflowing?
            {
				avgcolor += (endpixloc - float(pixel)) * retinaBuf[pixel * 4 + 2];
            	neuronactivation[blueneuron + i] = avgcolor / (xbluewidth * 255.0);
            	avgcolor = (1.0 - (endpixloc - float(pixel))) * retinaBuf[pixel * 4 + 2];
            	pixel++;
            }
        }
    }

#if IntegrateVision
	for( i = redneuron; i < redneuron + fNumRedNeurons + fNumGreenNeurons + fNumBlueNeurons; i++ )
		neuronactivation[i] = TauVision * neuronactivation[i]  +  (1.0 - TauVision) * newneuronactivation[i];
#endif

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating vision");
#endif DEBUGCHECK

#ifdef PRINTBRAIN
    if (printbrain && TSimulation::fOverHeadRank &&
        (critter::currentCritter == TSimulation::fMonitorCritter))
    {
        printf("***** age = %ld ****** overheadrank = %d ******\n", TSimulation::fAge, TSimulation::fOverHeadRank);
        printf("retinaBuf [0 - %d]\n",(brain::retinawidth - 1));
        printf("red:");
        
        for (i = 0; i < (brain::retinawidth * 4); i+=4)
            printf(" %3d", retinaBuf[i]);
        printf("\ngreen:");
        
        for (i = 1; i < (brain::retinawidth * 4); i+=4)
            printf(" %3d",retinaBuf[i]);
        printf("\nblue:");
        
        for (i = 2; i < (brain::retinawidth * 4); i+=4)
            printf(" %3d", retinaBuf[i]);
        printf("\n");
    }
#endif PRINTBRAIN

    for( i = firstnoninputneuron; i < numneurons; i++ )
    {
        newneuronactivation[i] = neuron[i].bias;
        for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
        {
            newneuronactivation[i] += synapse[k].efficacy *
               neuronactivation[abs(synapse[k].fromneuron)];
		}              
        newneuronactivation[i] = logistic(newneuronactivation[i], gLogisticsSlope);
    }

#define DebugBrain 0
#if DebugBrain
	static int numDebugBrains = 0;
	if( (TSimulation::fAge > 1) && (numDebugBrains < 10) )
	{
		numDebugBrains++;
//		if( numDebugBrains > 2 )
//			exit( 0 );
		printf( "***** age = %ld *****\n", TSimulation::fAge );
		printf( "random neuron [%d] = %g\n", randomneuron, neuronactivation[randomneuron] );
		printf( "energy neuron [%d] = %g\n", energyneuron, neuronactivation[energyneuron] );
		printf( "red neurons (%d):\n", fNumRedNeurons );
		for( i = 0; i < fNumRedNeurons; i++ )
			printf( "    %3d  %2d  %g\n", i+redneuron, i, neuronactivation[i+redneuron] );
		printf( "green neurons (%d):\n", fNumGreenNeurons );
		for( i = 0; i < fNumGreenNeurons; i++ )
			printf( "    %3d  %2d  %g\n", i+greenneuron, i, neuronactivation[i+greenneuron] );
		printf( "blue neurons (%d):\n", fNumBlueNeurons );
		for( i = 0; i < fNumBlueNeurons; i++ )
			printf( "    %3d  %2d  %g\n", i+blueneuron, i, neuronactivation[i+blueneuron] );
		printf( "output neurons (%d):\n", numOutputNeurons );
		for( i = firstOutputNeuron; i < firstInternalNeuron; i++ )
		{
			printf( "    %3d  %g  %g synapses (%ld):\n", i, neuron[i].bias, newneuronactivation[i], neuron[i].endsynapses - neuron[i].startsynapses );
			for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
				printf( "        %4ld  %2ld  %2d  %g  %g\n", k, k-neuron[i].startsynapses, synapse[k].fromneuron, synapse[k].efficacy, neuronactivation[abs(synapse[k].fromneuron)] );
		}
		printf( "internal neurons (%d):\n", numnoninputneurons-numOutputNeurons );
		for( i = firstInternalNeuron; i < numneurons; i++ )
		{
			printf( "    %3d  %g  %g synapses (%ld):\n", i, neuron[i].bias, newneuronactivation[i], neuron[i].endsynapses - neuron[i].startsynapses );
			for( k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++ )
				printf( "        %4ld  %2ld  %2d  %g  %g\n", k, k-neuron[i].startsynapses, synapse[k].fromneuron, synapse[k].efficacy, neuronactivation[abs(synapse[k].fromneuron)] );
		}
	}
#endif

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating neurons");
#endif DEBUGCHECK

#ifdef PRINTBRAIN
    if (printbrain && TSimulation::fOverHeadRank &&
        (critter::currentCritter == TSimulation::fMonitorCritter))
    {
        printf("  i neuron[i].bias neuronactivation[i] newneuronactivation[i]\n");
        for (i = 0; i < numneurons; i++)
            printf( "%3d  %1.4f  %1.4f  %1.4f\n", i, neuron[i].bias, neuronactivation[i], newneuronactivation[i] );
    }
#endif PRINTBRAIN

//	printf( "yaw activation = %g\n", newneuronactivation[yawneuron] );

    float learningrate;
    for (k = 0; k < numsynapses; k++)
    {
        if (synapse[k].toneuron >= 0) // 0 can't happen it's an input neuron
        {
            i = synapse[k].toneuron;
            ii = 0;
        }
        else
        {
            i = -synapse[k].toneuron;
            ii = 1;
        }
        if ( (synapse[k].fromneuron > 0) ||
            ((synapse[k].toneuron  == 0) && (synapse[k].efficacy >= 0.0)) )
        {
            j = synapse[k].fromneuron;
            jj = 0;
        }
        else
        {
            j = -synapse[k].fromneuron;
            jj = 1;
        }
        // Note: If .toneuron == 0, and .efficacy were to equal
        // 0.0 for an inhibitory synapse, we would choose the
        // wrong learningrate, but we prevent efficacy from going
        // to zero below & during initialization to prevent this.
        // Similarly, learningrate is guaranteed to be < 0.0 for
        // inhibitory synapses.
        learningrate = grouplrate[index4(neuron[i].group,neuron[j].group,ii,jj, numneurgroups,2,2)];
        synapse[k].efficacy += learningrate
                             * (newneuronactivation[i]-0.5f)
                             * (   neuronactivation[j]-0.5f);

        if (fabs(synapse[k].efficacy) > (0.5f * gMaxWeight))
        {
            synapse[k].efficacy *= 1.0f - (1.0f - gDecayRate) *
                (fabs(synapse[k].efficacy) - 0.5f * gMaxWeight) / (0.5f * gMaxWeight);
            if (synapse[k].efficacy > gMaxWeight)
                synapse[k].efficacy = gMaxWeight;
            else if (synapse[k].efficacy < -gMaxWeight)
                synapse[k].efficacy = -gMaxWeight;
        }
        else
        {
            // not strictly correct for this to be in an else clause,
            // but if lrate is reasonable, efficacy should never change
            // sign with a new magnitude greater than 0.5 * gMaxWeight
            if (learningrate >= 0.0f)  // excitatory
                synapse[k].efficacy = max(0.0f, synapse[k].efficacy);
            if (learningrate < 0.0f)  // inhibitory
                synapse[k].efficacy = min(-1.e-10f, synapse[k].efficacy);
        }
    }

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating synapses");
#endif DEBUGCHECK

#if 1
    for (i = firstnoninputneuron; i < numneurons; i++)
    {
        neuron[i].bias += groupblrate[neuron[i].group]
                        * (newneuronactivation[i]-0.5f)
                        * 0.5f;
        if (fabs(neuron[i].bias) > (0.5 * gNeuralValues.maxbias))
        {
            neuron[i].bias *= 1.0 - (1.0f - gDecayRate) *
                (fabs(neuron[i].bias) - 0.5f * gNeuralValues.maxbias) / (0.5f * gNeuralValues.maxbias);
            if (neuron[i].bias > gNeuralValues.maxbias)
                neuron[i].bias = gNeuralValues.maxbias;
            else if (neuron[i].bias < -gNeuralValues.maxbias)
                neuron[i].bias = -gNeuralValues.maxbias;
        }
    }

  #ifdef DEBUGCHECK
    debugcheck("brain::update after updating biases");
  #endif DEBUGCHECK
#endif

    float* saveneuronactivation = neuronactivation;
    neuronactivation = newneuronactivation;
    newneuronactivation = saveneuronactivation;
}


//---------------------------------------------------------------------------
// brain::Render
//---------------------------------------------------------------------------
#define DebugRender 0
#if DebugRender
	#define rPrint( x... ) printf( x )
#else
	#define rPrint( x... )
#endif
void brain::Render(short patchwidth, short patchheight)
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
    for (i = 0, x1 = 2 * patchwidth; i < short(numneurons); i++, x1+=patchwidth)
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
    for (i = firstnoninputneuron, y1 = 2*patchheight; i < numneurons; i++, y1 += patchheight)
    {
        const unsigned char mag = (unsigned char)((gMaxWeight + neuron[i].bias) * 127.5 / gMaxWeight);
		glColor3ub(mag, mag, mag);
        glRecti(x1, y1, x2, y1 + patchheight);
    }

    // this vertical row of elements shows the new states in all the
    // unclamped neurons, including the output neurons
    x1 = x2;
    x2 = x1 + patchwidth;
    for (i = firstnoninputneuron, y1 = 2*patchheight; i < numneurons;
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
            xoff + short(numneurons) * patchwidth,
            yoff + short(numnoninputneurons) * patchheight);
	
	glLineWidth( 1.0 );	
	rPrint( "**************************************************************\n");
    for (k = 0; k < numsynapses; k++)
    {
        const unsigned char mag = (unsigned char)((gMaxWeight + synapse[k].efficacy) * 127.5 / gMaxWeight);

		// Fill the rect
		glColor3ub(mag, mag, mag);
        x1 = xoff  +   abs(synapse[k].fromneuron) * patchwidth;
        y1 = yoff  +  (abs(synapse[k].toneuron)-firstnoninputneuron) * patchheight;   

		if( abs( synapse[k].fromneuron ) < firstInternalNeuron )	// input or output neuron, so it can be both excitatory and inhibitory
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
				k, synapse[k].efficacy, mag, x1, y1, abs(synapse[k].fromneuron), abs(synapse[k].toneuron), firstOutputNeuron, firstInternalNeuron );
	}
	
	
	//
    // Now highlight the neuronal groups for clarity
    //
    
    // Red
    y1 = patchheight;
    y2 = y1 + patchheight;

    x1 = short(redneuron)*patchwidth + xoff;
    x2 = x1 + fNumRedNeurons*patchwidth;

	glColor3ub(255, 0, 0);
	glLineWidth(1.0);	
 	glBegin(GL_LINE_LOOP);
		glVertex2i(x1, y1);
		glVertex2i(x2 - 1, y1);
		glVertex2i(x2 - 1, y2 + 1);
		glVertex2i(x1, y2);        	
	glEnd();        

	// Green
    x1 = x2;
    x2 = x1 + fNumGreenNeurons * patchwidth;
	glColor3ub(0, 255, 0);
	glLineWidth(1.0);	
 	glBegin(GL_LINE_LOOP);
		glVertex2i(x1, y1);
		glVertex2i(x2 - 1, y1);
		glVertex2i(x2 - 1, y2 + 1);
		glVertex2i(x1, y2);        	
	glEnd();        

	// Blue
    x1 = x2;
    x2 = x1 + fNumBlueNeurons * patchwidth;
	glColor3ub(0, 0, 255);
	glLineWidth(1.0);
 	glBegin(GL_LINE_LOOP);
		glVertex2i(x1, y1);
		glVertex2i(x2, y1);
		glVertex2i(x2, y2 + 1);
		glVertex2i(x1, y2);        	
	glEnd();        
	
	// Frame the groups
	glColor3ub(255, 255, 255);
	glLineWidth(1.0);	
    x2 = numinputneurons * patchwidth + xoff;
    for (i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)
    {
		short numneur;
	
	#if DesignerBrains
		// For DesignerBrains, the genes are meaningless, but the architectures
		// are all the same, so we can use the class static firsteneur to figure things out
		if( i < numneurgroups - 1 )
			numneur = firsteneur[i+1] - firsteneur[i];
		else
			numneur = numneurons - firsteneur[i] + 1;
	#else
		numneur = mygenes->numneurons(i);
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
    
    for (i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)
    {
		short numneur;
		
	#if DesignerBrains
		// For DesignerBrains, the genes are meaningless, but the architectures
		// are all the same, so we can use the class static firsteneur to figure things out
		if( i < numneurgroups - 1 )
			numneur = firsteneur[i+1] - firsteneur[i];
		else
			numneur = numneurons - firsteneur[i] + 1;
	#else
		numneur = mygenes->numneurons(i);
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

