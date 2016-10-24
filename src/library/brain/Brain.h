#pragma once

// Global
#include <istream>
#include <ostream>
#include <string>

// Local
#include "NeuralNetRenderer.h"
#include "NeuronModel.h"
#include "proplib/proplib.h"
#include "utils/objectlist.h"


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
			TAU_GAIN,
			SPIKING
		} neuronModel;
		enum
		{
			LEARN_NONE,
			LEARN_PREBIRTH,
			LEARN_ALL
		} learningMode;
		struct
		{
			float minVal;
			float maxVal;
			float seedVal;
		} Tau;
		struct
		{
			float minVal;
			float maxVal;
			float seedVal;
		} Gain;
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
		bool fixedInitWeight;
		bool gaussianInitWeight;
		float gaussianInitMaxStdev;
		float maxWeight;
		float initMaxWeight;
		bool enableLearning;
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
	NeuronModel::Dimensions getDimensions();
	NeuronModel *getNeuronModel();
        
	void getActivations( double *activations, int start, int count );
	void setActivations( double *activations, int start, int count );
	void randomizeActivations();

	bool isFrozen();
	void freeze();
	void unfreeze();

	void dumpAnatomical( AbstractFile *file, long index, float fitness );
	
	void startFunctional( AbstractFile *file, long index );
	void endFunctional( AbstractFile* file, float fitness );
	void writeFunctional( AbstractFile* file );

	void dumpSynapses( AbstractFile *file, long index );
	void loadSynapses( AbstractFile *file, float maxWeight = -1.0f );
	void copySynapses( Brain *other );
	
protected:
	friend class agent;

	NervousSystem *_cns;
	NeuronModel::Dimensions _dims;
	NeuronModel *_neuralnet;
	NeuralNetRenderer *_renderer;
	float _energyUse;
	bool _frozen;
};

//===========================================================================
// inlines
//===========================================================================
inline float Brain::getEnergyUse() { return _energyUse; }
inline short Brain::getNumNeurons() { return _dims.numNeurons; }
inline long Brain::getNumSynapses() { return _dims.numSynapses; }
inline NeuronModel::Dimensions Brain::getDimensions() { return _dims; }
inline NeuronModel *Brain::getNeuronModel() { return _neuralnet; }
inline void Brain::getActivations( double *activations, int start, int count ) { _neuralnet->getActivations( activations, start, count ); }
inline void Brain::setActivations( double *activations, int start, int count ) { _neuralnet->setActivations( activations, start, count ); }
inline void Brain::randomizeActivations() { _neuralnet->randomizeActivations(); }
inline bool Brain::isFrozen() { return _frozen; }
inline void Brain::freeze() { _frozen = true; }
inline void Brain::unfreeze() { _frozen = false; }

// Macros
