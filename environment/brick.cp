/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// brick.cp - implementation of bricks

// Self
#include "brick.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "critter.h"
#include "globals.h"
#include "graphics.h"


using namespace std;

// External globals
float brick::gBrickHeight;
Color brick::gBrickColor;

//===========================================================================
// bricks
//===========================================================================


//-------------------------------------------------------------------------------------------
// brick::brick
//-------------------------------------------------------------------------------------------
brick::brick()
{
	setType( BRICKTYPE );
	initBrick();
}


//-------------------------------------------------------------------------------------------
// brick::brick
//-------------------------------------------------------------------------------------------
brick::brick( float x, float z )
{
	initBrick( x, z );
}


//-------------------------------------------------------------------------------------------
// brick::~brick
//-------------------------------------------------------------------------------------------
brick::~brick()
{
}


//-------------------------------------------------------------------------------------------
// brick::dump
//-------------------------------------------------------------------------------------------
void brick::dump( ostream& out )
{
    out << fPosition[0] sp fPosition[1] sp fPosition[2] nl;
}


//-------------------------------------------------------------------------------------------
// brick::load
//-------------------------------------------------------------------------------------------
void brick::load(istream& in)
{
	float x, y, z;
	
    in >> x >> y >> z;

    initBrick( x, y, z );
}



//-------------------------------------------------------------------------------------------
// brick::initBrick
//-------------------------------------------------------------------------------------------
void brick::initBrick()
{
	initBrick( randpw() * globals::worldsize, randpw() * globals::worldsize );
}


//-------------------------------------------------------------------------------------------
// brick::initBrick
//-------------------------------------------------------------------------------------------
void brick::initBrick( float x, float z )
{
	initBrick( x, 0.5 * gBrickHeight, z );
}
 

//-------------------------------------------------------------------------------------------
// brick::initBrick
//-------------------------------------------------------------------------------------------
void brick::initBrick( float x, float y, float z )
{
	fPosition[0] = x;
	fPosition[1] = y;
	fPosition[2] = z;
	
	setlen( gBrickHeight, gBrickHeight, gBrickHeight );
	
	setcolor( gBrickColor );
}
 

#if 0
//-------------------------------------------------------------------------------------------
// brick::setradius
//-------------------------------------------------------------------------------------------           
void brick::setradius()
{
	if( !fRadiusFixed )  //  only set radius anew if not set manually
		fRadius = sqrt( fLength[0]*fLength[0] + fLength[2]*fLength[2] ) * fRadiusScale * fScale * 0.5;
	srPrint( "brick::%s(): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}
#endif
