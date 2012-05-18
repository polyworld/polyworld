#pragma once

// Global
#include <istream>
#include <ostream>
#include <string>

// Local
#include "NeuralNetRenderer.h"
#include "NeuronModel.h"
#include "objectlist.h"
#include "proplib.h"


#define DebugBrainGrow false
#if DebugBrainGrow
	extern bool DebugBrainGrowPrint;
#endif

#define PrintBrain false
#if PrintBrain
	#define BPRINT(x...) if(bprint) {printf(x);}
	#define IF_BPRINT(stmt) if(bprint) stmt
	#define IF_BPRINTED(stmt) if(bprint && !bprinted) {bprinted = true; stmt;}
#else
	#define BPRINT(x...)
	#define IF_BPRINT(stmt)
	#define IF_BPRINTED(stmt)
#endif

#define DebugRender 0
#if DebugRender
	#define rPrint( x... ) printf( x )
#else
	#define rPrint( x... )
#endif

// Forward declarations
class AbstractFile;
class agent;
namespace genome { class Genome; }
class NervousSystem;
class NeuronModel;

//===========================================================================
// Brain
//===========================================================================
class Brain
{	
public:
	static struct Configuration
	{
		enum
		{
			Groups,
			Sheets
		} architecture;
		enum
		{
			FIRING_RATE,
			TAU,
			SPIKING
		} neuronModel;
		struct
		{
			float minVal;
			float maxVal;
			float seedVal;
		} Tau;
		struct
		{
			bool enableGenes;
			double aMinVal;
			double aMaxVal;
			double bMinVal;
			double bMaxVal;
			double cMinVal;
			double cMaxVal;
			double dMinVal;
			double dMaxVal;
		} Spiking;
		float maxbias;
		bool outputSynapseLearning;
		bool synapseFromOutputNeurons;
		long numPrebirthCycles;
		float logisticSlope;
		float maxWeight;
		float initMaxWeight;
		float minlrate;
		float maxlrate;
		float decayRate;
		short minWin;
		short retinaWidth;
		short retinaHeight;
		float maxsynapse2energy; // (amount if all synapses usable)
		float maxneuron2energy;
	} config;

	static void processWorldfile( proplib::Document &doc );
	static void init();

    Brain( NervousSystem *cns );
    virtual ~Brain();
    
	void prebirth();
    void update( bool bprint );

	NeuralNetRenderer *getRenderer();

    float getEnergyUse();
    short getNumNeurons();
	long  getNumSynapses();
        
	void dumpAnatomical( AbstractFile *file, long index, float fitness );
	
	void startFunctional( AbstractFile *file, long index );
	void endFunctional( AbstractFile* file, float fitness );
	void writeFunctional( AbstractFile* file );
	
protected:
	friend class agent;

	NervousSystem *_cns;
	NeuronModel::Dimensions _dims;
	NeuronModel *_neuralnet;
	NeuralNetRenderer *_renderer;
	float _energyUse;
};

//===========================================================================
// inlines
//===========================================================================
inline float Brain::getEnergyUse() { return _energyUse; }
inline short Brain::getNumNeurons() { return _dims.numNeurons; }
inline long Brain::getNumSynapses() { return _dims.numSynapses; }

// Macros
