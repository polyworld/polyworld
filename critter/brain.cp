/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// Self
#include "brain.h"

// qt
#include <qapplication.h>

// Local
#include "critter.h"
#include "debug.h"
#include "misc.h"


//#define DEBUGBRAINGROW


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
unsigned char* brain::gRetinaBuf;
short brain::gMinWin;

static float initminweight = 0.0; // could read this in

#ifdef PRINTBRAIN
	extern bool printbrain;
	extern short overheadrank;
	critter* currentcritter;
	critter* monitorcritter;
	bool brainprinted = false;
	extern critter* curfittestcrit[5];
#endif PRINTBRAIN

// External globals
NeuralValues brain::gNeuralValues;
long brain::gNumPrebirthCycles;
float brain::gLogisticsSlope;
float brain::gMaxWeight;
float brain::gInitMaxWeight;
float brain::gDecayRate;

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
        
	brain::gRetinaBuf = (unsigned char *)calloc(brain::retinawidth * 4, sizeof(unsigned char));
//	cout << "allocated brain::gRetinaBuf of 4 * width bytes, where width" ses brain::retinawidth nl;
    Q_CHECK_PTR(gRetinaBuf);
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
	free(brain::gRetinaBuf);
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
// brain::Grow
//---------------------------------------------------------------------------
void brain::Grow(genome* g)
{
#ifdef DEBUGCHECK
    debugcheck("brain::grow entry");
#endif DEBUGCHECK

    mygenes = g;

    numneurgroups = g->NumNeuronGroups();

#ifdef DEBUGBRAINGROW
    cout << "****************************************************" nlf;
    cout << "Starting a new brain with numneurgroups = " << numneurgroups nlf;
#endif DEBUGBRAINGROW

    numinputneurons = 0;
    
    short i;
    for (i = 0; i < brain::gNeuralValues.numinputneurgroups; i++)
    {
        firsteneur[i] = numinputneurons;
        firstineur[i] = numinputneurons; // input neurons double as e & i
        numinputneurons += g->numeneur(i);

#ifdef DEBUGBRAINGROW
        cout << "group " << i << " has " << g->numeneur(i) << " neurons" nlf;
#endif DEBUGBRAINGROW

    }
    firstnoninputneuron = numinputneurons;

#ifdef DEBUGBRAINGROW
    cout << "numinputneurons = " << numinputneurons nlf;
#endif DEBUGBRAINGROW

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

#ifdef DEBUGBRAINGROW
    cout << "fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = "
         << fNumRedNeurons cms fNumGreenNeurons cms fNumBlueNeurons nlf;;
#endif DEBUGBRAINGROW

    numsynapses = 0;
    numnoninputneurons = 0;
    
    short j, ii;
    for (i = brain::gNeuralValues.numinputneurgroups, ii = 0; i < numneurgroups; i++, ii++)
    {
        firsteneur[i] = numinputneurons + numnoninputneurons;
        if (i < (numneurgroups - brain::gNeuralValues.numoutneurgroups))//output neurons are both e & i
            numnoninputneurons += g->numeneur(i);

        firstineur[i] = numinputneurons + numnoninputneurons;
        numnoninputneurons += g->numineur(i);

#ifdef DEBUGBRAINGROW
        cout << "group " << i << " has " << g->numeneur(i) << " e-neurons" nlf;
        cout << "  and " << i << " has " << g->numineur(i) << " i-neurons" nlf;
#endif DEBUGBRAINGROW

        for (j = 0; j < numneurgroups; j++)
        {
            numsynapses += g->numsynapses(i,j);

#ifdef DEBUGBRAINGROW
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
#endif DEBUGBRAINGROW

        }
    }
    
    numneurons = numnoninputneurons + numinputneurons;
    if (numneurons > brain::gNeuralValues.maxneurons)
        error(2,"numneurons (",numneurons,") > maxneurons (", brain::gNeuralValues.maxneurons,") in brain::grow");
        
    if (numsynapses > brain::gNeuralValues.maxsynapses)
        error(2,"numsynapses (",numsynapses,") > maxsynapses (", brain::gNeuralValues.maxsynapses,") in brain::grow");

    // set up the ouput/behavior neurons as the last numoutneur neurons
    focusneuron = numneurons - 1;
    lightneuron = focusneuron - 1;
    yawneuron = lightneuron - 1;
    speedneuron = yawneuron - 1;
    fightneuron = speedneuron - 1;
    mateneuron = fightneuron - 1;
    eatneuron = mateneuron - 1;

#ifdef DEBUGBRAINGROW
    cout << "numneurons = " << numneurons << "  (of " << brain::gNeuralValues.maxneurons pnlf;
    cout << "numsynapses = " << numsynapses << "  (of " << brain::gNeuralValues.maxsynapses pnlf;
#endif DEBUGBRAINGROW

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
#ifdef DEBUGBRAINGROW
        cout << "For group " << i << ":" nlf;
#endif DEBUGBRAINGROW

        float groupbias = g->Bias(i);
        groupblrate[i] = g->BiasLearningRate(i);

#ifdef DEBUGBRAINGROW
        cout << "  groupbias = " << groupbias nlf;
        cout << "  groupbiaslearningrate = " << groupblrate[i] nlf;
#endif DEBUGBRAINGROW

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

#ifdef DEBUGBRAINGROW
        cout << "  Setting up " << nneuri << " e-neurons" nlf;
#endif DEBUGBRAINGROW
		
		short ini;	
        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firsteneur[i];

#ifdef DEBUGBRAINGROW
            cout << "  For ini, ineur = "
                 << ini cms ineur << ":" nlf;
#endif DEBUGBRAINGROW

            neuron[ineur].group = i;
            neuron[ineur].bias = groupbias;
            neuron[ineur].startsynapses = numsyn;

#ifdef DEBUGBRAINGROW
            cout << "    group = " << neuron[ineur].group nlf;
            cout << "    bias = " << neuron[ineur].bias nlf;
            cout << "    startsynapses = " << neuron[ineur].startsynapses nlf;
            cout << "    Setting up e-e connections:" nlf;
#endif DEBUGBRAINGROW

            // setup all e-e connections for this e-neuron
            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numeneur(j);

#ifdef DEBUGBRAINGROW
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)eeremainder = "
                     << nneurj cms eeremainder[j] nlf;
#endif DEBUGBRAINGROW

                nsynij = g->numeesynapses(i,j);
                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + eeremainder[j] + 1.e-5);
                eeremainder[j] += nsynijperneur - newsyn;
                tdij = g->eetd(i,j);

                joff = short((float(ini) / float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#ifdef DEBUGBRAINGROW
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)eeremainder, tdij, joff = "
                     << eeremainder[j] cms tdij cms joff nlf;
#endif DEBUGBRAINGROW

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
                        if (i == j) // same group and neuron type
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ini);
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

#ifdef DEBUGBRAINGROW
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy, lrate = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif DEBUGBRAINGROW

                    numsyn++;
                }
            }

            // setup all i-e connections for this e-neuron

#ifdef DEBUGBRAINGROW
            cout << "    Setting up i-e connections:" nlf;
#endif DEBUGBRAINGROW

            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numineur(j);

#ifdef DEBUGBRAINGROW
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)ieremainder = "
                     << nneurj cms ieremainder[j] nlf;
#endif DEBUGBRAINGROW

                nsynij = g->numiesynapses(i,j);
                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + ieremainder[j] + 1.e-5);
                ieremainder[j] += nsynijperneur - newsyn;
                tdij = g->ietd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj)
                     - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#ifdef DEBUGBRAINGROW
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)ieremainder, tdij, joff = "
                     << ieremainder[j] cms tdij cms joff nlf;
#endif DEBUGBRAINGROW

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
                        if ((i==j)&&(i==(numneurgroups-1)))//same & output group
                            jneur = NearestFreeNeuron(jneur, &neurused[0], nneurj, ini);
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

#ifdef DEBUGBRAINGROW
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy, lrate = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif DEBUGBRAINGROW

                    numsyn++;
                }
            }

            neuron[ineur].endsynapses = numsyn;
            numneur++;
        }

        // setup all i-neurons for this group

        if (i >= (numneurgroups - brain::gNeuralValues.numoutneurgroups))
            nneuri = 0;  // output/behavior neurons are e-only postsynaptically
        else
            nneuri = g->numineur(i);

#ifdef DEBUGBRAINGROW
        cout << "  Setting up " << nneuri << " i-neurons" nlf;
#endif DEBUGBRAINGROW

        for (ini = 0; ini < nneuri; ini++)
        {
            ineur = ini + firstineur[i];

#ifdef DEBUGBRAINGROW
            cout << "  For ini, ineur = "
                 << ini cms ineur << ":" nlf;
#endif DEBUGBRAINGROW

            neuron[ineur].group = i;
            neuron[ineur].bias = groupbias;
            neuron[ineur].startsynapses = numsyn;

#ifdef DEBUGBRAINGROW
            cout << "    group = " << neuron[ineur].group nlf;
            cout << "    bias = " << neuron[ineur].bias nlf;
            cout << "    startsynapses = " << neuron[ineur].startsynapses nlf;
            cout << "    Setting up e-i connections:" nlf;
#endif DEBUGBRAINGROW

            // setup all e-i connections for this i-neuron

            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numeneur(j);

#ifdef DEBUGBRAINGROW
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)eiremainder = "
                     << nneurj cms eiremainder[j] nlf;
#endif DEBUGBRAINGROW

                nsynij = g->numeisynapses(i, j);
                nsynijperneur = float(nsynij) / float(nneuri);
                newsyn = short(nsynijperneur + eiremainder[j] + 1.e-5);
                eiremainder[j] += nsynijperneur - newsyn;
                tdij = g->eitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#ifdef DEBUGBRAINGROW
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)eiremainder, tdij, joff = "
                     << eiremainder[j] cms tdij cms joff nlf;
#endif DEBUGBRAINGROW

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
                        if ((i==j)&&(i==(numneurgroups-1)))//same & output group
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
                        synapse[numsyn].efficacy = rrand(initminweight, gInitMaxWeight);

#ifdef DEBUGBRAINGROW
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy, lrate = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif DEBUGBRAINGROW

                    numsyn++;
                }
            }

            // setup all i-i connections for this i-neuron
            for (j = 0; j < numneurgroups; j++)
            {
                nneurj = g->numineur(j);

#ifdef DEBUGBRAINGROW
                cout << "      From group " << j nlf;
                cout << "      with nneurj, (old)iiremainder = "
                     << nneurj cms iiremainder[j] nlf;
#endif DEBUGBRAINGROW

                nsynij = g->numiisynapses(i,j);
                nsynijperneur = float(nsynij)/float(nneuri);
                newsyn = short(nsynijperneur + iiremainder[j] + 1.e-5);
                iiremainder[j] += nsynijperneur - newsyn;
                tdij = g->iitd(i,j);

                joff = short((float(ini)/float(nneuri)) * float(nneurj) - float(newsyn) * 0.5);
                joff = max<short>(0, min<short>(nneurj - newsyn, joff));

#ifdef DEBUGBRAINGROW
                cout << "      and nsynij, nsynijperneur, newsyn = "
                     << nsynij cms nsynijperneur cms newsyn nlf;
                cout << "      and (new)iiremainder, tdij, joff = "
                     << iiremainder[j] cms tdij cms joff nlf;
#endif DEBUGBRAINGROW

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
                        synapse[numsyn].efficacy = min(-1.e-10, -rrand(initminweight, gInitMaxWeight));

#ifdef DEBUGBRAINGROW
                    cout << "        synapse[" << numsyn
                         << "].toneur, fromneur, efficacy, lrate = "
                         << ineur cms jneur cms synapse[numsyn].efficacy nlf;
#endif DEBUGBRAINGROW

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
        neuronactivation[i] = 0.5;

    energyuse = brain::gNeuralValues.maxneuron2energy * float(numneurons) / float(brain::gNeuralValues.maxneurons)
              + brain::gNeuralValues.maxsynapse2energy * float(numsynapses) / float(brain::gNeuralValues.maxsynapses);

#ifdef DEBUGCHECK
    debugcheck("brain::grow after setting up architecture");
#endif DEBUGCHECK

    // now send some signals through the system
    // try pure noise for now...
    for (i = 0; i < gNumPrebirthCycles; i++)
    {
        // load up the retinabuf with noise
        for (j = 0; j < (brain::retinawidth * 4); j++)
            gRetinaBuf[j] = (unsigned char)(rrand(0.0, 255.0));
        Update(drand48());
    }

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
    if (printbrain && overheadrank && !brainprinted && currentcritter == monitorcritter)
    {
        brainprinted = true;
        printf("neuron (toneuron)  fromneuron   synapse   efficacy\n");
        
        for (i = firstnoninputneuron; i < numneurons; i++)
        {
            for (k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++)
            {
				printf("%3d   %3d    %3d    %5d    %f\n",
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
                avgcolor += gRetinaBuf[(pixel++) * 4 + 3];
            neuronactivation[redneuron+i] = avgcolor / (xredwidth * 255.0);
        }
    }
    else
    {
        pixel = 0;
        avgcolor = 0.0;
#ifdef PRINTBRAIN
        if (printbrain && (currentcritter == monitorcritter))
        {
            printf("xredwidth = %f\n", xredwidth);
        }
#endif PRINTBRAIN
        for (i = 0; i < fNumRedNeurons; i++)
        {
            endpixloc = xredwidth * float(i+1);
#ifdef PRINTBRAIN
            if (printbrain &&
                (currentcritter == monitorcritter))
            {
                printf("  neuron %d, endpixloc = %g\n", i, endpixloc);
            }
#endif PRINTBRAIN
            while (float(pixel) < (endpixloc - 1.0))
            {
                avgcolor += gRetinaBuf[(pixel++) * 4 + 3];
#ifdef PRINTBRAIN
                if (printbrain && (currentcritter == monitorcritter))
                {
                    printf("    in loop with pixel %d, avgcolor = %g\n", pixel,avgcolor);
					if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc - 1.0 -1.0e - 5)))
						printf("Got in-loop borderline case - red\n");
                }
#endif PRINTBRAIN
            }
            
            avgcolor += (endpixloc - float(pixel)) * gRetinaBuf[pixel * 4 + 3];
            neuronactivation[redneuron + i] = avgcolor / (xredwidth * 255.0);
#ifdef PRINTBRAIN
            if (printbrain && (currentcritter == monitorcritter))
            {
                printf("    after loop with pixel %d, avgcolor = %g, color = %g\n", pixel,avgcolor,neuronactivation[redneuron+i]);
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0 + 1.0e - 5)))
                    printf("Got outside-loop borderline case - red\n");
            }
#endif PRINTBRAIN
            avgcolor = (1.0 - (endpixloc - float(pixel))) * gRetinaBuf[pixel * 4 + 3];
#ifdef PRINTBRAIN
            if (printbrain && (currentcritter == monitorcritter))
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
                avgcolor += gRetinaBuf[(pixel++) * 4 + 2];
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
                avgcolor += gRetinaBuf[(pixel++) * 4 + 2];
#ifdef PRINTBRAIN
                if (printbrain && (currentcritter == monitorcritter))
                {
                    if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc -1.0 -1.0e - 5)) )
                        printf("Got in-loop borderline case - green\n");
                }
#endif PRINTBRAIN
            }
#ifdef PRINTBRAIN
            if (printbrain && (currentcritter == monitorcritter))
            {
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0)))
                    printf("Got outside-loop borderline case - green\n");
            }
#endif PRINTBRAIN
            avgcolor += (endpixloc - float(pixel)) * gRetinaBuf[pixel * 4 + 2];
            neuronactivation[greenneuron + i] = avgcolor / (xgreenwidth * 255.0);
            avgcolor = (1.0 - (endpixloc - float(pixel))) * gRetinaBuf[pixel * 4 + 2];
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
                avgcolor += gRetinaBuf[(pixel++) * 4 + 1];
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
			while (float(pixel) < (endpixloc-1.+1.e-5))
            {
                avgcolor += gRetinaBuf[(pixel++) * 4 + 1];
#ifdef PRINTBRAIN
                if (printbrain && (currentcritter == monitorcritter))
                {
                    if ((float(pixel) < (endpixloc - 1.0)) && (float(pixel) >= (endpixloc - 1.0 - 1.0e - 5)) )
                        printf("Got in-loop borderline case - blue\n");
                }
#endif PRINTBRAIN
            }
            
#ifdef PRINTBRAIN
            if (printbrain && (currentcritter == monitorcritter))
            {
                if ((float(pixel) >= (endpixloc - 1.0)) && (float(pixel) < (endpixloc - 1.0 + 1.0e - 5)) )
                    printf("Got outside-loop borderline case - blue\n");
            }
#endif PRINTBRAIN

            if (pixel < brain::retinawidth)  // TODO How do we end up overflowing?
            {
				avgcolor += (endpixloc - float(pixel)) * gRetinaBuf[pixel * 4 + 1];
            	neuronactivation[blueneuron + i] = avgcolor / (xbluewidth * 255.0);
            	avgcolor = (1.0 - (endpixloc - float(pixel))) * gRetinaBuf[pixel * 4 + 1];
            	pixel++;
            }
        }
    }

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating vision");
#endif DEBUGCHECK

#ifdef PRINTBRAIN
    if (printbrain && overheadrank &&
        (currentcritter == monitorcritter))
    {
        printf("***** age = %ld ****** overheadrank = %d ******\n", globals::age, overheadrank);
        printf("gRetinaBuf [0 - %d]\n",(brain::retinawidth - 1));
        printf("red:");
        
        for (i = 3; i < (brain::retinawidth * 4); i+=4)
            printf(" %3d", gRetinaBuf[i]);
        printf("\ngreen:");
        
        for (i = 2; i < (brain::retinawidth * 4); i+=4)
            printf(" %3d",gRetinaBuf[i]);
        printf("\nblue:");
        
        for (i = 1; i < (brain::retinawidth * 4); i+=4)
            printf(" %3d", gRetinaBuf[i]);
        printf("\n");
    }
#endif PRINTBRAIN

    for (i = firstnoninputneuron; i < numneurons; i++)
    {
        newneuronactivation[i] = neuron[i].bias;
        for (k = neuron[i].startsynapses; k < neuron[i].endsynapses; k++)
        {
            newneuronactivation[i] += synapse[k].efficacy *
               neuronactivation[abs(synapse[k].fromneuron)];
		}              
        newneuronactivation[i] = logistic(newneuronactivation[i], gLogisticsSlope);
    }

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating neurons");
#endif DEBUGCHECK

#ifdef PRINTBRAIN
    if (printbrain && overheadrank &&
        (currentcritter == monitorcritter))
    {
        printf("  i neuron[i].bias neuronactivation[i] newneuronactivation[i]\n");
        for (i = 0; i < numneurons; i++)
            printf("%3d  %1.4f  %1.4f  %1.4f\n",
                i,neuron[i].bias,neuronactivation[i],newneuronactivation[i]);
    }
#endif PRINTBRAIN

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
            synapse[k].efficacy *= 1.0f + (gDecayRate - 1.0f)*
                (fabs(synapse[k].efficacy)-0.5f * gMaxWeight) / (0.5f * gMaxWeight);
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
            if (learningrate > 0.0f)  // excitatory
                synapse[k].efficacy = max(0.0f, synapse[k].efficacy);
            if (learningrate < 0.0f)  // inhibitory
                synapse[k].efficacy = min(-1.e-10f, synapse[k].efficacy);
        }
    }

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating synapses");
#endif DEBUGCHECK

    for (i = firstnoninputneuron; i < numneurons; i++)
    {
        neuron[i].bias += groupblrate[neuron[i].group]
                        * (newneuronactivation[i]-0.5f)
                        * 0.5f;
        if (fabs(neuron[i].bias) > (0.5 * gMaxWeight))
        {
            neuron[i].bias *= 1.0 + (gDecayRate - 1.0f) *
                (fabs(neuron[i].bias)-0.5f * gMaxWeight) / (0.5f * gMaxWeight);
            if (neuron[i].bias > gMaxWeight)
                neuron[i].bias = gMaxWeight;
            else if (neuron[i].bias < -gMaxWeight)
                neuron[i].bias = -gMaxWeight;
        }
    }

#ifdef DEBUGCHECK
    debugcheck("brain::update after updating biases");
#endif DEBUGCHECK

    float* saveneuronactivation = neuronactivation;
    neuronactivation = newneuronactivation;
    newneuronactivation = saveneuronactivation;
}


//---------------------------------------------------------------------------
// brain::Render
//---------------------------------------------------------------------------
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
    for (i = 0, x1 = 2 * patchheight; i < short(numneurons); i++, x1+=patchwidth)
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
    x2 = patchheight;
    for (i = 0, y1 = 2*patchheight; i < numnoninputneurons; i++, y1 += patchwidth)
    {
        const unsigned char mag = (unsigned char)((gMaxWeight + neuron[i].bias) * 127.5 / gMaxWeight);
		glColor3ub(mag, mag, mag);
        glRecti(x1, y1, x2, y1 + patchwidth);
    }

    // this vertical row of elements shows the new states in all the
    // unclamped neurons, including the output neurons
    x1 = x2;
    x2 = x1 + patchheight;
    for (i = short(firstnoninputneuron), y1 = 2*patchheight; i < numneurons;
         i++, y1 += patchwidth)
    {
        const unsigned char mag = (unsigned char)(neuronactivation[i] * 255.);
		glColor3ub(mag, mag, mag);
        glRecti(x1, y1, x2, y1 + patchwidth);
    }

    // this array of synaptic strengths unfortunately shows the new, updated
    // values rather than those actually used to map the displayed horizontal
    // row of neuronal values onto the displayed vertical row of values
    xoff = 2 * patchheight;
    yoff = 2 * patchheight;

	glColor3ub(127, 127, 127);
    glRecti(xoff,
            yoff,
            xoff + short(numneurons) * patchwidth,
            yoff + short(numnoninputneurons) * patchwidth);

    for (k = 0; k < numsynapses; k++)
    {
        const unsigned char mag = (unsigned char)((gMaxWeight + synapse[k].efficacy) * 127.5 / gMaxWeight);

		// Fill the rect
		glColor3ub(mag, mag, mag);
        x1 = xoff + abs(synapse[k].fromneuron)*patchwidth;
        y1 = yoff + (abs(synapse[k].toneuron)-firstnoninputneuron)*patchwidth;        
		glRecti(x1, y1, x1 + patchwidth, y1 + patchwidth);
		
		// Now frame it
		glColor3ub(0, 0, 0);
        glLineWidth(1.0);	
	 	glBegin(GL_LINE_LOOP);
	        	glVertex2i(x1, y1);
	        	glVertex2i(x1 + patchwidth, y1);
	        	glVertex2i(x1 + patchwidth, y1 + patchwidth + 1);
	        	glVertex2i(x1, y1 + patchwidth);        	
		glEnd();       
    }
	
	
	//
    // Now highlight the input and output neurons for clarity
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
	
	// Frame the cells
	glColor3ub(255, 255, 255);
	glLineWidth(1.0);	
    x2 = numinputneurons * patchwidth + xoff;
    for (i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)
    {
        x1 = x2;
        x2 = x1 + (mygenes->numneurons(i)) * patchwidth;
        
        glBegin(GL_LINE_LOOP);
        	glVertex2i(x1, y1);
        	glVertex2i(x2, y1);
        	glVertex2i(x2, y2 + 1);
        	glVertex2i(x1, y2);        	
        glEnd();
    }

    x1 = patchheight;
    x2 = x1 + patchheight;
    y2 = yoff;
    
    for (i = brain::gNeuralValues.numinputneurgroups; i < numneurgroups; i++)
    {
        y1 = y2;
        y2 = y1 + (mygenes->numneurons(i)) * patchwidth;
        
		glBegin(GL_LINE_LOOP);
        	glVertex2i(x1, y1);
        	glVertex2i(x2, y1);
        	glVertex2i(x2, y2 + 1);
        	glVertex2i(x1, y2);        	
        glEnd();
    }
}

