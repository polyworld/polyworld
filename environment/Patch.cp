
// Patch.cp - implementation of generic patches

// Self
#include "Patch.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "agent.h"
#include "globals.h"
#include "graphics.h"
#include "food.h"
#include "distributions.h"
#include "Simulation.h"

using namespace std;


//===========================================================================
// Patch
//===========================================================================

// Default constructor
Patch::Patch(){
}

//-------------------------------------------------------------------------------------------
// Patch::Patch
//-------------------------------------------------------------------------------------------
void Patch::initBase(float x, float z, float sx, float sz, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber){

	centerX = dm->startX  +  x * dm->absoluteSizeX;
	centerZ = dm->startZ  +  z * dm->absoluteSizeZ; 
   
	sizeX = sx * dm->absoluteSizeX;
	sizeZ = sz * dm->absoluteSizeZ;

	startX = centerX  -  sizeX * 0.5;
	endX = startX + sizeX;

	startZ = centerZ  -  sizeZ * 0.5;
	endZ = startZ + sizeZ;

	areaShape = shape;
	distribution = distrib;

	neighborhoodSize = nhsize;

	agentInsideCount = 0;
	agentNeighborhoodCount = 0;

	fStage = fs;

	domainNumberOfParent = domainNumber;
}

//-------------------------------------------------------------------------------------------
// Patch::~Patch
//-------------------------------------------------------------------------------------------
Patch::~Patch()
{
}


void Patch::printPatch()
{
    printf("Patch at (%.2f, %.2f) with size (%.2f, %.2f)\n", centerX, centerZ, sizeX, sizeZ);
}



// Get a new point inside the patch based on the shape and distribution of the Patch
void  Patch::setPoint( float* x, float* z )
{
	// Set these values in worldfile?
	float sigma = .3; // normal distribution std. deviation
	float mu = 0.5;  // normal distribution mean
	float slope = -0.4;  // linear distribution slope
	float yIntercept = 0.4;  // linear distribution yIntercept

	if( areaShape == RECTANGULAR )
	{
		if( distribution == UNIFORM )
		{
		    *x = startX  +  sizeX * randpw();
		    *z = startZ  +  sizeZ * randpw();
		}
		else if( distribution == LINEAR )
		{
		    *x = startX  +  sizeX * getLinear( slope, yIntercept );
		    *z = startZ  +  sizeZ * getLinear( slope, yIntercept );
		}
		else if( distribution == GAUSSIAN )
		{
		    *x = startX  +  sizeX * getNormal( sigma, mu );
		    *z = startZ  +  sizeZ * getNormal( sigma, mu );
		}
	}

	// Check for points outside of the ellipse and keep generating points until
	// we get one inside the ellipse
	else if( areaShape == ELLIPTICAL )
	{
		float a = sizeX / 2.0;
		float b = sizeZ / 2.0;
		if( distribution == UNIFORM )
		{
			*x = startX  +  sizeX * randpw();
			*z = startZ  +  sizeZ * randpw();
			while( ((*x - centerX) * (*x - centerX) / (a * a) + (*z - centerZ) * (*z - centerZ) / (b * b)) > 1.0 )
			{
				*x = startX  +  sizeX * randpw();
				*z = startZ  +  sizeZ * randpw();
			}
		}
		else if( distribution == LINEAR )
		{
			*x = startX  +  sizeX * getLinear( slope, yIntercept );
			*z = startZ  +  sizeZ * getLinear( slope, yIntercept );
			while( ((*x - centerX) * (*x - centerX) / (a * a) + (*z - centerZ) * (*z - centerZ) / (b * b)) > 1.0 )
			{
				*x = startX  +  sizeX * getLinear( slope, yIntercept );
				*z = startZ  +  sizeZ * getLinear( slope, yIntercept );
			}
		}
		else if( distribution == GAUSSIAN )
		{
			*x = startX  +  sizeX * getNormal( sigma, mu );
			*z = startZ  +  sizeZ * getNormal( sigma, mu );
			while( ((*x - centerX) * (*x - centerX) / (a * a) + (*z - centerZ) * (*z - centerZ) / (b * b)) > 1.0 )
			{
				*x = startX  +  sizeX * getNormal( sigma, mu );
				*z = startZ  +  sizeZ * getNormal( sigma, mu );
			}
		}
	}
	else {
		printf( "Illegal patch shape: %d\n", areaShape );
		exit( 1 );
	}
}



// Reset the counts each step in the simulation
void Patch::resetAgentCounts()
{
	agentInsideCount = 0;
	agentNeighborhoodCount = 0;
}


// Get the total area of the patch
// Used to generate this patch's "fraction" when it is not 
// explicitly specified in the worldfile
float Patch::getArea()
{
	if( areaShape == RECTANGULAR )
	{
		return( sizeX * sizeZ );
	}
	else 
	{
		return( PI * (sizeX * 0.5) * (sizeZ * 0.5) );
	}

}

// Check if point (x, z) is inside the patch boundary + outerRange
// Use outerRange 0 to check if point is inside patch
// Use outerRange >0 to add a neighborhood to the patch
bool Patch::pointIsInside( float x, float z, float outerRange )
{
	// Easy case of a rectangular Patch
	if( areaShape == RECTANGULAR )
	{
		if( (x >= startX - outerRange) && (x <= endX + outerRange) &&
			(z >= startZ - outerRange) && (z <= endZ + outerRange) )
			return true;
		else 
			return false;
	}
	else   // More difficult case of an elliptical Patch
	{
		// Calculate values for the major and minor axes of the ellipse
		float a = sizeX * 0.5  +  outerRange;
		float b = sizeZ * 0.5  +  outerRange;

		// Check using formula for an ellipse
		if( ( (x - centerX) * (x - centerX) / (a * a) + (z - centerZ) * (z - centerZ) / (b * b) ) <= 1.0 )
			return true;
		else
			return false;
	}
}

// Check if a agent with given x and z coordinates is inside the patch
// Increment the Patch's agentInsideCount if it is
void Patch::checkIfAgentIsInside( float agentX, float agentZ )
{
	// Check if point is inside the Patch (outerRange is 0 to get only those agents that are truly inside)
	if( pointIsInside( agentX, agentZ, 0 ) )
		agentInsideCount++;
}


// Check if a agent with given x and z coordinates is inside the Patch's neighborhood
// Increment the Patch's agentOuterRangeCount if it is
void Patch::checkIfAgentIsInsideNeighborhood( float agentX, float agentZ )
{
	// Check if point is inside outerRange AND outside of Patch
	if( pointIsInside( agentX, agentZ, neighborhoodSize ) && !pointIsInside( agentX, agentZ, 0 ) )
		agentNeighborhoodCount++;
}
