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
#include "critter.h"
#include "globals.h"
#include "graphics.h"


using namespace std;

// External globals
fxsortedlist food::gXSortedFood;
float food::gFoodHeight;
Color food::gFoodColor;
float food::gMinFoodEnergy;
float food::gMaxFoodEnergy;
float food::gSize2Energy;


//===========================================================================
// food
//===========================================================================


//-------------------------------------------------------------------------------------------
// food::food
//-------------------------------------------------------------------------------------------
food::food()
{
	initfood();
}


//-------------------------------------------------------------------------------------------
// food::food
//-------------------------------------------------------------------------------------------
food::food(float e)
{
	initfood(e);
}


//-------------------------------------------------------------------------------------------
// food::food
//-------------------------------------------------------------------------------------------
food::food(float e, float x, float z)
{
	initfood(e, x, z);
}


//-------------------------------------------------------------------------------------------
// food::~food
//-------------------------------------------------------------------------------------------
food::~food()
{
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
// food::initfood
//-------------------------------------------------------------------------------------------
void food::initfood()
{
	fEnergy = drand48() * (gMaxFoodEnergy - gMinFoodEnergy) + gMinFoodEnergy;
	initlen();
	initpos();
	initrest();
}


//-------------------------------------------------------------------------------------------
// food::initfood
//-------------------------------------------------------------------------------------------
void food::initfood(float e)
{
	fEnergy = e;
	initlen();
	initpos();
	initrest();
}


//-------------------------------------------------------------------------------------------
// food::initfood
//-------------------------------------------------------------------------------------------
void food::initfood(float e, float x, float z)
{
	fEnergy = e;
	initlen();
	fPosition[0] = x;
	fPosition[1] = 0.5 * fLength[1];
	fPosition[2] = z;
	initrest();
}
 

//-------------------------------------------------------------------------------------------
// food::initpos
//-------------------------------------------------------------------------------------------    
void food::initpos()
{
	fPosition[0] = drand48() * globals::worldsize;
	fPosition[1] = 0.5 * fLength[1];
	fPosition[2] = drand48() * -globals::worldsize;
}


//-------------------------------------------------------------------------------------------
// food::initlen
//-------------------------------------------------------------------------------------------       
void food::initlen()
{
	float lxz = 0.75 * fEnergy / gSize2Energy;
	float ly = gFoodHeight;
	setlen(lxz,ly,lxz);
}


//-------------------------------------------------------------------------------------------
// food::initrest
//-------------------------------------------------------------------------------------------           
void food::initrest()
{
	setcolor(gFoodColor);
}


//-------------------------------------------------------------------------------------------
// food::setenergy
//-------------------------------------------------------------------------------------------           
void food::setenergy(float e)
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


//-------------------------------------------------------------------------------------------
// fxsortedlist::add
//-------------------------------------------------------------------------------------------           
void fxsortedlist::add(food* a)
{
    bool inserted = false;
    food* o;
	
    this->reset();
    while (this->next(o))
    {
        if ((a->x()-a->radius()) < (o->x()-o->radius()))
        {
            this->inserthere(a);
            inserted = true;
            break;
        }
    }
    if (!inserted)
		this->append(a);
}


//-------------------------------------------------------------------------------------------
// fxsortedlist::sort
//-------------------------------------------------------------------------------------------           
void fxsortedlist::sort()
{
// This technique assumes that the list is almost entirely sorted at the start
// Hopefully, with some reasonable frame-to-frame coherency, this will be true!
// Actually, food is static, except for newly grown food, for now.
    gdlink<food*> *savecurr;
	
    food* o = NULL;
    food* p = NULL;
    food* b = NULL;
    this->reset();
    this->next(p);
    savecurr = currItem;
    while (this->next(o))
    {
        if ((o->x()-o->radius()) < (p->x()-p->radius()))
        {
            gdlink<food*> *link = this->unlink();  // at o, just unlink it
            currItem = savecurr;  // back up to previous one directly
            while (this->prev(b)) // then use iterator to move back again
                if ((b->x()-b->radius()) < (o->x()-o->radius()))
                    break; // until we have one that starts before o
            if (currItem)  // we did find one, and didn't run off beginning of list
                this->appendhere(link);
            else  // we have a new head of the list
                this->insert(link);
            currItem = savecurr;
            o = p;
        }
        p = o;
        savecurr = currItem;
    }
}

