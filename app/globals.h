/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// globals.h - global variables for pw (PolyWorld)

#ifndef GLOBALS_H
#define GLOBALS_H

// System
#include <stdio.h>

// qt
#include <qevent.h>

// Local
#include "AbstractFile.h"
#include "GenomeLayout.h"
#include "graphics.h"

// Forward declarations
class agent;

// Custom events
const int kUpdateEventType = QEvent::User + 1;

static const int kMenuBarHeight = 22;

// [TODO] 
static const long XMAXSCREEN = 1024;
static const long YMAXSCREEN = 768;
static const long xscreensize = XMAXSCREEN + 1;
static const long yscreensize = YMAXSCREEN + 1;
static const int MAXLIGHTS = 10;

// Prefs
//static const char kPrefPath[] = "edu.indiana.polyworld";
//static const char kPrefSection[] = "polyworld";
//static const char kAppSettingsName[] = "polyworld";
static const char kWindowsGroupSettingsName[] = "windows";

class globals
{
public:
	static float	worldsize;
	static bool		wraparound;
	static bool		edges;
	static AbstractFile::ConcreteFileType recordFileType;
};

//===========================================================================
// brain
//===========================================================================
namespace brain
{

	struct NeuralValues
	{
		enum NeuronModel
		{
			FIRING_RATE,
			TAU,
			SPIKING
		} model;
		short minneurons;
		short maxneurons;
		long maxsynapses;
		short numinputneurgroups;
		short numoutneurgroups;
		struct
		{
			float minVal;
			float maxVal;
			float seedVal;
		} Tau;
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

		bool enableTopologicalDistortionRngSeed;
		long minTopologicalDistortionRngSeed;
		long maxTopologicalDistortionRngSeed;

		float maxsynapse2energy; // (amount if all synapses usable)
		float maxneuron2energy;
	};

	// Public globals	
    extern NeuralValues gNeuralValues;
    extern long gNumPrebirthCycles;
	extern float gLogisticsSlope;
	extern float gMaxWeight;
	extern bool gEnableInitWeightRngSeed;
	extern long gMinInitWeightRngSeed;
	extern long gMaxInitWeightRngSeed;
	extern float gInitMaxWeight;
	extern float gDecayRate;
	extern short gMinWin;
	extern short retinawidth;
	extern short retinaheight;

} // namespace brain


//===========================================================================
// genome
//===========================================================================
namespace genome
{
    // External globals
    extern long gNumBytes;
	extern GenomeLayout::LayoutType gLayoutType;

	extern float gSeedFightBias;
	extern float gSeedFightExcitation;

	extern bool gEnableMateWaitFeedback;
	extern bool gEnableSpeedFeedback;
	extern bool gEnableGive;
	extern float gSeedGiveBias;

	extern bool gEnableCarry;
	extern float gSeedPickupBias;
	extern float gSeedDropBias;
	extern float gSeedPickupExcitation;
	extern float gSeedDropExcitation;

	extern bool gEnableVisionPitch;

    extern float gMinStrength;
    extern float gMaxStrength;
	extern float gMinMutationRate;
//	extern float genome::gMaxMutationRate;
	extern float gMaxMutationRate;
	extern long gMinNumCpts;
	extern long gMaxNumCpts;
	extern long gMinLifeSpan;
	extern long gMaxLifeSpan;
	extern float gMiscBias;
	extern float gMiscInvisSlope;
	extern float gMinBitProb;
	extern float gMaxBitProb;
	extern bool gGrayCoding;	
	extern short gMinvispixels;
	extern short gMaxvispixels;
	extern float gMinmateenergy;
	extern float gMaxmateenergy;
	extern float gMinmaxspeed;
	extern float gMaxmaxspeed;
	extern float gMinlrate;
	extern float gMaxlrate;
}

#endif

