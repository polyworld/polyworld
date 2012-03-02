#pragma once

#include "FittestList.h"
#include "FoodPatch.h"
#include "simtypes.h"

using namespace sim;

//===========================================================================
// Domain
//===========================================================================
class Domain
{
 public:
	Domain();
	~Domain();
	
    float centerX;
    float centerZ;
    float absoluteSizeX;
    float absoluteSizeZ;
    float sizeX;
    float sizeZ;
    float startX;
    float startZ;
    float endX;
    float endZ;
    int numFoodPatches;
    int numBrickPatches;
    float fraction;
    float foodRate;
    int foodCount;
    int initFoodCount;
    int minFoodCount;
    int maxFoodCount;
    int maxFoodGrownCount;
	int numFoodPatchesGrown;
	FoodPatch* fFoodPatches;
	class BrickPatch* fBrickPatches;
    long minNumAgents;
    long maxNumAgents;
    long initNumAgents;
	long numberToSeed;
    long numAgents;
    long numcreated;
    long numborn;
    long numbornsincecreated;
    long numdied;
    long lastcreate;
    long maxgapcreate;
	long numToCreate;
	float probabilityOfMutatingSeeds;
	double energyScaleFactor;
    short ifit;
    short jfit;
    FittestList *fittest;	// based on complete fitness, however it is being calculated in AgentFitness(c)
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	class agent** fLeastFit;	// based on heuristic fitness

	FoodPatch* whichFoodPatch( float x, float z );
};

inline Domain::Domain()
{
	foodCount = 0;
}

inline Domain::~Domain()
{
}

inline FoodPatch* Domain::whichFoodPatch( float x, float z )
{
	FoodPatch* fp = NULL;
	
	for( int i = 0; i < numFoodPatches; i++ )
	{
		if( fFoodPatches[i].pointIsInside( x, z, 0.0 ) )
		{
			fp = &(fFoodPatches[i]);
			break;
		}
	}
	
	return( fp );
}
