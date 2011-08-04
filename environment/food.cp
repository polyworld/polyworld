/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// food.C - implementation of food classes

// Self
#include "food.h"

// System
#include <ostream>

// qt
#include <qapplication.h>

// Local
#include "agent.h"
#include "globals.h"
#include "graphics.h"


using namespace std;

// Static class variables
unsigned long food::fFoodEver;

// External globals
float food::gFoodHeight;
Color food::gFoodColor;
float food::gMinFoodEnergy;
float food::gMaxFoodEnergy;
float food::gSize2Energy;
float food::gMaxFoodRadius;
float food::gCarryFood2Energy;
long food::gMaxLifeSpan;
food::FoodList food::gAllFood;

//===========================================================================
// food
//===========================================================================


//-------------------------------------------------------------------------------------------
// food::food
//-------------------------------------------------------------------------------------------
food::food( long step )
{
	initfood( step );
}


//-------------------------------------------------------------------------------------------
// food::food
//-------------------------------------------------------------------------------------------
food::food( long step, float e )
{
	initfood( step, e );
}


//-------------------------------------------------------------------------------------------
// food::food
//-------------------------------------------------------------------------------------------
food::food( long step, float e, float x, float z )
{
	initfood( step, e, x, z );
}


//-------------------------------------------------------------------------------------------
// food::~food
//-------------------------------------------------------------------------------------------
food::~food()
{
	assert( *fAllFoodIterator == this );
	gAllFood.erase( fAllFoodIterator );
}


//-------------------------------------------------------------------------------------------
// food::dump
//-------------------------------------------------------------------------------------------
void food::dump(ostream& out)
{
    out << fEnergy nl;
    out << fPosition[0] sp fPosition[1] sp fPosition[2] nl;
}


//-------------------------------------------------------------------------------------------
// food::load
//-------------------------------------------------------------------------------------------
void food::load(istream& in)
{
    in >> fEnergy;
    in >> fPosition[0] >> fPosition[1] >> fPosition[2];

    initlen();
}


//-------------------------------------------------------------------------------------------
// food::eat
//-------------------------------------------------------------------------------------------
float food::eat(float e)
{
	float er = e < fEnergy ? e : fEnergy;
	fEnergy -= er;
	initlen();
	
	return er;
}


//-------------------------------------------------------------------------------------------
// food::getAge
//-------------------------------------------------------------------------------------------
long food::getAge(long step)
{
	return step - fCreationStep;
}


//-------------------------------------------------------------------------------------------
// food::initfood
//-------------------------------------------------------------------------------------------
void food::initfood( long step )
{
	float e = randpw() * (gMaxFoodEnergy - gMinFoodEnergy) + gMinFoodEnergy;
	initfood( step, e );
}


//-------------------------------------------------------------------------------------------
// food::initfood
//-------------------------------------------------------------------------------------------
void food::initfood( long step, float e )
{
	fEnergy = e;
	float x = randpw() * globals::worldsize;
	float z = randpw() * globals::worldsize;
	initfood( step, e, x, z );
}


//-------------------------------------------------------------------------------------------
// food::initfood
//-------------------------------------------------------------------------------------------
void food::initfood( long step, float e, float x, float z )
{
	fEnergy = e;
	initlen();
	fPosition[0] = x;
	fPosition[1] = 0.5 * fLength[1];
	fPosition[2] = z;
	initrest();

	fCreationStep = step;
	gAllFood.push_back( this );
	fAllFoodIterator = --gAllFood.end();
	assert( *fAllFoodIterator == this );
}
 

//-------------------------------------------------------------------------------------------
// food::initlen
//-------------------------------------------------------------------------------------------       
void food::initlen()
{
	float lxz = 0.75 * fEnergy / gSize2Energy;
	float ly = gFoodHeight;
	setlen( lxz, ly, lxz );
}


//-------------------------------------------------------------------------------------------
// food::initrest
//-------------------------------------------------------------------------------------------           
void food::initrest()
{
	setType( FOODTYPE );
	setTypeNumber( ++food::fFoodEver );
	setcolor( gFoodColor );
}


//-------------------------------------------------------------------------------------------
// food::setenergy
//-------------------------------------------------------------------------------------------           
void food::setenergy( float e )
{
	fEnergy = e;
	initlen();
}


//-------------------------------------------------------------------------------------------
// food::setradius
//-------------------------------------------------------------------------------------------           
void food::setradius()
{
	if( !fRadiusFixed )  //  only set radius anew if not set manually
		fRadius = sqrt( fLength[0]*fLength[0] + fLength[2]*fLength[2] ) * fRadiusScale * fScale * 0.5;
	srPrint( "food::%s(): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}
