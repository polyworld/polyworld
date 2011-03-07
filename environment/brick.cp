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
#include "agent.h"
#include "globals.h"
#include "graphics.h"


using namespace std;

// External globals
float brick::gBrickHeight;
float brick::gBrickRadius;
unsigned long brick::NumBricks;
bool brick::BrickClassInited;
float brick::gCarryBrick2Energy;


//===========================================================================
// bricks
//===========================================================================


//-------------------------------------------------------------------------------------------
// brick::brick
//-------------------------------------------------------------------------------------------
brick::brick( Color color )
{
	initBrick( color );
}


//-------------------------------------------------------------------------------------------
// brick::brick
//-------------------------------------------------------------------------------------------
brick::brick( Color color, float x, float z )
{
	initBrick( color, x, z );
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
	
	assert( false ); // need to store color
    //initBrick( x, y, z );
}



//-------------------------------------------------------------------------------------------
// brick::initBrick
//-------------------------------------------------------------------------------------------
void brick::initBrick( Color color )
{
	initBrick( color, randpw() * globals::worldsize, randpw() * globals::worldsize );
}


//-------------------------------------------------------------------------------------------
// brick::initBrick
//-------------------------------------------------------------------------------------------
void brick::initBrick( Color color, float x, float z )
{
	initBrick( color, x, 0.5 * gBrickHeight, z );
}
 

//-------------------------------------------------------------------------------------------
// brick::initBrick
//-------------------------------------------------------------------------------------------
void brick::initBrick( Color color, float x, float y, float z )
{
	if( !BrickClassInited )
		InitBrickClass();
	
	NumBricks++;
	
	setType( BRICKTYPE );
	setTypeNumber( NumBricks );

	fPosition[0] = x;
	fPosition[1] = y;
	fPosition[2] = z;
	
	setlen( gBrickHeight, gBrickHeight, gBrickHeight );
	
	setcolor( color );
}
 

//-------------------------------------------------------------------------------------------
// brick::InitBrickClass
//-------------------------------------------------------------------------------------------
void brick::InitBrickClass()
{
	if( BrickClassInited )
		return;
	
	BrickClassInited = true;
	NumBricks = 0;
	gBrickRadius = 0.5 * sqrt( 2.0 ) * gBrickHeight;
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
