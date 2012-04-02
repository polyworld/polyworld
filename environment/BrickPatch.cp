
// BrickPatch.cp - implementation of brick patches
// Self
#include "BrickPatch.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "agent.h"
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
void BrickPatch::init(Color color, float x, float z, float sx, float sz, int numberBricks, int shape, int distrib, float nhsize, gstage* fs, Domain* dm, int domainNumber, bool on)
{
	initBase(x, z,  sx, sz, shape, distrib, nhsize, fs, dm, domainNumber);
	brickCount = numberBricks;
	brickColor = color;

	onPrev = false;
	this->on = on;
}

//-------------------------------------------------------------------------------------------
// BrickPatch::~BrickPatch
//-------------------------------------------------------------------------------------------
BrickPatch::~BrickPatch()
{
}

void BrickPatch::updateOn()
{
	if( on != onPrev )
	{
		if( on )
			addBricks();
		else
			removeBricks();

		onPrev = on;
	}
}

void BrickPatch::addBricks()
{
	for( int i = 0; i < brickCount; i++ )
	{
		float x, z;
		brick* b = new brick( brickColor );

		// set the values of x and y to a legal point in the BrickPatch
		setPoint(&x, &z);

		b->setx(x);
		b->setz(z);

		b->setPatch(this);
		objectxsortedlist::gXSortedObjects.add(b);
		fStage->AddObject(b); 
	}
}

void BrickPatch::removeBricks()
{
	brick *b;

	list<brick *> removeList;

	// There are patches currently needing removal, so do it
	objectxsortedlist::gXSortedObjects.reset();
	while( objectxsortedlist::gXSortedObjects.nextObj( BRICKTYPE, (gobject**) &b ) )
	{
		if( b->myBrickPatch == this )
		{
			objectxsortedlist::gXSortedObjects.removeCurrentObject();   // get it out of the list
			fStage->RemoveObject( b );
			if( b->BeingCarried() )
			{
				// if it's being carried, have to remove it from the carrier
				((agent*)(b->CarriedBy()))->DropObject( (gobject*) b );
			}
		}
	}
}
