
// BrickPatch.cp - implementation of brick patches
// Self
#include "BrickPatch.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "critter.h"
#include "globals.h"
#include "graphics.h"
#include "brick.h"
#include "distributions.h"
#include "Simulation.h"


using namespace std;


//===========================================================================
// BrickPatch
//===========================================================================


// Default constructor
BrickPatch::BrickPatch(){
}

//-------------------------------------------------------------------------------------------
// BrickPatch::BrickPatch
//-------------------------------------------------------------------------------------------
void BrickPatch::init(float x, float z, float sx, float sz, int numberBricks, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber)
{
	initBase(x, z,  sx, sz, shape, distrib, nhsize, fs, dm, domainNumber);
	brickCount = numberBricks;
}

//-------------------------------------------------------------------------------------------
// BrickPatch::~BrickPatch
//-------------------------------------------------------------------------------------------
BrickPatch::~BrickPatch()
{
}

// Add a new brick somewhere inside this BrickPatch
void BrickPatch::addBrick(){
    float x, z;
    brick* b = new brick;

    // set the values of x and y to a legal point in the BrickPatch
    setPoint(&x, &z);

    b->setx(x);
    b->setz(z);

    b->setPatch(this);
    objectxsortedlist::gXSortedObjects.add(b);
    fStage->AddObject(b); 
}

