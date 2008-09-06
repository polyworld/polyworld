#ifndef BRAIN_H
#define BRAIN_H

// Global
#include <istream>
#include <ostream>

// Local
#include "objectlist.h"

#define USE_BIAS				true
#define SPIKING_MODEL			false
#define SpikingActivation       25.0
#define BIAS_INJECTED_VOLTAGE   208.0
#define STDP_RESET				.1
#define STDP_DEGRADATION_SCALER .95
#define STDP_DEPRESSION_SCALER	.5
#define BrainStepsPerWorldStep  50
#define MaxFiringRatePerSecond  260.0
#define MinFiringRatePerSecond  0.0
#define SpikingParameter_a      0.02
#define SpikingParameter_b      0.2
#define SpikingParameter_c      -65
#define SpikingParameter_d      6
#define NumOutputNeurons 7

struct NeuralValues
{
	short minneurons;
	short maxneurons;
	long maxsynapses;
	short numinputneurgroups;
	short numoutneurgroups;
	short mininternalneurgroups;
	short maxinternalneurgroups;
	short mineneurpergroup;
	short maxeneurpergroup;
	short minineurpergroup;
	short maxineurpergroup;
	short maxneurpergroup;
	short maxneurgroups;
	short maxnoninputneurgroups;
	short maxinternalneurons;
	short maxinputneurons;
	short maxnoninputneurons;
	float maxbias;
	float minbiaslrate;
	float maxbiaslrate;
	float minconnectiondensity;
	float maxconnectiondensity;
	float mintopologicaldistortion;
	float maxtopologicaldistortion;
	float maxsynapse2energy; // (amount if all synapses usable)
	float maxneuron2energy;
};


// note: activation levels are not maintained in the neuronstruct
// so that after the new activation levels are computed, the old
// and new blocks of memory can simply be repointered rather than
// copied.
struct neuronstruct
{
	short group;
    float bias;
    long  startsynapses;
    long  endsynapses;
#if SPIKING_MODEL	
	float v;              //!<represents the membrane potential of the neuron 
	float u;			  //!<the membranes recovery period			
	float STDP;           //!<spike-timing-dependent plasticity,
	short maxfiringcount; //explain later if works
#endif
};


struct synapsestruct
{
    float efficacy;   // > 0 for excitatory, < 0 for inhibitory
    short fromneuron; // > 0 for excitatory, < 0 for inhibitory
    short toneuron;   // > 0 for excitatory, < 0 for inhibitory
#if SPIKING_MODEL
	float delta;  //!from iz intead of effecting weights directly
#endif	
};


// Forward declarations
class critter;
class genome;


//===========================================================================
// brain
//===========================================================================
class brain
{	
public:
	static void braininit();
	static void braindestruct();
	
    brain();
    ~brain();
    
    void Dump(std::ostream& out);
    void Load(std::istream& in);
    void Report();
    void Grow( genome* g, long critterNumber, bool recordBrainAnatomy );
    void Update(float energyfraction);
	void UpdateSpikes(float energyfraction, FILE* fHandle);	

    float Random();
    float Eat();
    float Mate();
    float Fight();
    float Speed();
    float Yaw();
    float Light();
    float Focus();
    float BrainEnergy();
    short NumVisPixels();
    short GetNumNeurons();
    short GetNumNonInputNeurons();
    short NumRedNeurons();
    short NumGreenNeurons();
    short NumBlueNeurons();
	short NumNeuronGroups();
        
	void dumpAnatomical( const char* directoryName, const char* suffix, long index, float fitness );
	FILE* startFunctional( long index );
	void endFunctional( FILE* file, float fitness );
	void writeFunctional( FILE* file );
        
    void Render(short patchwidth, short patchheight);
	
	// Public globals	
    static NeuralValues gNeuralValues;
    static long gNumPrebirthCycles;
	static float gLogisticsSlope;
	static float gMaxWeight;
	static float gInitMaxWeight;
	static float gDecayRate;
	static short gMinWin;
	static short retinawidth;
	static short retinaheight;

	// Each critter/brain gets its own retinaBuf, because they all see different things,
	// and we only want to do the glReadPixels once (each)
	unsigned char* retinaBuf;
	float scale_latest_spikes;

protected:
	friend class critter;
	
    static bool classinited;
    static short* firsteneur;	// [maxneurgroups]
    static short* firstineur; 	// [maxneurgroups]
    static float* eeremainder;	// [maxneurgroups]
    static float* eiremainder;	// [maxneurgroups]
    static float* iiremainder;	// [maxneurgroups]
    static float* ieremainder;	// [maxneurgroups]
    static bool* neurused; 		// [max(maxeneurpergroup,maxineurpergroup)]
    static short randomneuron;
    static short energyneuron;
       
    short numinputneurons;
    short numnoninputneurons;
    short numneurons;
    long numsynapses;
    genome* mygenes;
    short redneuron;
    short greenneuron;
    short blueneuron;
    short eatneuron;
    short mateneuron;
    short fightneuron;
    short speedneuron;
    short yawneuron;
    short lightneuron;
    short focusneuron;
    float* groupblrate;
    float* grouplrate;
	float* outputActivation;
	neuronstruct* neuron;
    synapsestruct* synapse;
    float* neuronactivation;
    float* newneuronactivation;
    short numneurgroups;
    short fNumRedNeurons;
    short fNumGreenNeurons;
    short fNumBlueNeurons;
    short firstnoninputneuron;
	short firstOutputNeuron;
	short numOutputNeurons;
	short firstInternalNeuron;
    float xredwidth;
    float xgreenwidth;
    float xbluewidth;
    short xredintwidth;
    short xgreenintwidth;
    short xblueintwidth;
    float energyuse;
	short neuronsize;
	short neuronactivationsize;
	long synapsesize;
    
    void AllocateBrainMemory();
    short NearestFreeNeuron(short iin, bool* used, short num, short exclude);
	
	void GrowDesignedBrain( genome* g );
	float DesignedEfficacy( short toGroup, short fromGroup, short isyn, int synapseType );
};

//===========================================================================
// inlines
//===========================================================================
inline float brain::Random() { return neuronactivation[randomneuron]; }
inline float brain::Eat() { return neuronactivation[eatneuron]; }
inline float brain::Mate() { return neuronactivation[mateneuron]; }
inline float brain::Fight() { return neuronactivation[fightneuron]; }
inline float brain::Speed() { return neuronactivation[speedneuron]; }
inline float brain::Yaw() { return neuronactivation[yawneuron]; }
inline float brain::Light() { return neuronactivation[lightneuron]; }
inline float brain::Focus() { return neuronactivation[focusneuron]; }
inline float brain::BrainEnergy() { return energyuse; }
inline short brain::NumVisPixels() { return max(max(fNumRedNeurons,fNumGreenNeurons),fNumBlueNeurons); }
inline short brain::GetNumNeurons() { return numneurons; }
inline short brain::GetNumNonInputNeurons() { return numnoninputneurons; }
inline short brain::NumRedNeurons() { return fNumRedNeurons; }
inline short brain::NumGreenNeurons() { return fNumGreenNeurons; }
inline short brain::NumBlueNeurons() { return fNumBlueNeurons; }
inline short brain::NumNeuronGroups() { return numneurgroups; }

// Macros

#define IsInputNeuralGroup( group ) ( (group < brain::gNeuralValues.numinputneurgroups) && (group >= 0) )
#define IsOutputNeuralGroup( group ) ( (group >= brain::gNeuralValues.numinputneurgroups) && (group < (brain::gNeuralValues.numinputneurgroups + brain::gNeuralValues.numoutneurgroups)) )
// Note: Following upper-limit check is only a coarse sanity check, rather than a strict upper limit based on the current brain or genome, but using this form allows us to use define these macros for use everywhere
#define IsInternalNeuralGroup( group ) ( (group >= (brain::gNeuralValues.numinputneurgroups + brain::gNeuralValues.numoutneurgroups)) && (group < (brain::gNeuralValues.numinputneurgroups + brain::gNeuralValues.numoutneurgroups + brain::gNeuralValues.maxinternalneurgroups)) )

#endif
