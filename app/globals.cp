/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// globals.cp - global variables for pw (PolyWorld)

// Self
#include "globals.h"

float   globals::worldsize;		// agent, food
bool	globals::wraparound;	// agent, sceneview, simulation
bool	globals::edges;			// agent, simulation
bool	globals::stickyEdges;
AbstractFile::ConcreteFileType globals::recordFileType;

namespace brain
{

    NeuralValues gNeuralValues;
    long gNumPrebirthCycles;
	float gLogisticsSlope;
	float gMaxWeight;
	bool gEnableInitWeightRngSeed;
	long gMinInitWeightRngSeed;
	long gMaxInitWeightRngSeed;
	float gInitMaxWeight;
	float gDecayRate;
	short gMinWin;
	short retinawidth;
	short retinaheight;

}

namespace genome
{
	GenomeLayout::LayoutType gLayoutType;

	float gSeedFightBias;
	float gSeedFightExcitation;

	bool gEnableMateWaitFeedback;
	bool gEnableSpeedFeedback;

	bool gEnableGive;
	float gSeedGiveBias;

	bool gEnableCarry;
	float gSeedPickupBias;
	float gSeedDropBias;
	float gSeedPickupExcitation;
	float gSeedDropExcitation;

	bool gEnableVisionPitch;
	bool gEnableVisionYaw;

	float gMinStrength;
	float gMaxStrength;
	float gMinMutationRate;
	float gMaxMutationRate;
	long gMinNumCpts;
	long gMaxNumCpts;
	long gMinLifeSpan;
	long gMaxLifeSpan;
	float gMiscBias;
	float gMiscInvisSlope;
	float gMinBitProb;
	float gMaxBitProb;
	bool gGrayCoding;
	short gMinvispixels;
	short gMaxvispixels;
	float gMinmateenergy;
	float gMaxmateenergy;
	float gMinmaxspeed;
	float gMaxmaxspeed;
	float gMinlrate;
	float gMaxlrate;
}
