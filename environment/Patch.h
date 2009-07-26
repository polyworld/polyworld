//---------------------------------------------------------------------------
//	File:		Patch.h
//---------------------------------------------------------------------------


#ifndef PATCH_H
#define PATCH_H

//System
#include <iostream>
#include "gstage.h"

#define RECTANGULAR 0
#define ELLIPTICAL 1

#define UNIFORM 0
#define LINEAR 1
#define GAUSSIAN 2

using namespace std;


// Forward declarations
class Patch;
class Domain;

//===========================================================================
// Patch
//===========================================================================
class Patch
{
 public:
	Patch();
	~Patch();
    
	float centerX;	// in normalized 0.0 to 1.0 coords
	float centerZ;	// in normalized 0.0 to 1.0 coords

	float startX;	// in absolute coords
	float startZ;	// in absolute coords

	float endX;		// in absolute coords
	float endZ;		// in absolute coords

	float sizeX;	// in normalized 0.0 to 1.0 coords
	float sizeZ;	// in normalized 0.0 to 1.0 coords

	int areaShape;
	int distribution;
       
	int agentInsideCount;
	int agentNeighborhoodCount;

	float neighborhoodSize;
    
	gstage* fStage;

	int domainNumberOfParent;

	void setPoint(float* x, float* z);
	void resetAgentCounts();
	float getArea();
	bool pointIsInside(float x, float z, float outerRange);
	void checkIfAgentIsInside(float agentX, float agentZ);
	void checkIfAgentIsInsideNeighborhood(float agentX, float agentZ);
	void initBase(float x, float z, float sx, float sz, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber);
    
 protected:
    
	void printPatch();
    
};




#endif
