/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// barrier.cp - implementation of barrier classes

// Self
#include "barrier.h"

// System
#include <stdlib.h>

// qt
#include <qapplication.h>

// Local
#include "globals.h"
#include "graphics.h"

// barrier globals
bxsortedlist barrier::gXSortedBarriers;
float barrier::gBarrierHeight;
Color barrier::gBarrierColor;


//===========================================================================
// barrier
//===========================================================================

//---------------------------------------------------------------------------
// barrier::barrier
//---------------------------------------------------------------------------
barrier::barrier()
{
	init(0., 0., 0., -1.);
}


//---------------------------------------------------------------------------
// barrier::barrier
//---------------------------------------------------------------------------
barrier::barrier(float xa, float za, float xb, float zb)
{
	init(xa, za, xb, zb);
}


//---------------------------------------------------------------------------
// barrier::barrier
//---------------------------------------------------------------------------
barrier::~barrier()
{
	if (fVertices != NULL)
		delete fVertices;
}


//---------------------------------------------------------------------------
// barrier::init
//---------------------------------------------------------------------------
void barrier::init(float xa, float za, float xb, float zb)
{
	fNumPoints = 4;
	setcolor(gBarrierColor);
	fVertices = new float[12];
  
	if (fVertices == NULL)
		error(1,"Insufficient memory to setup barrier vertices");
	else
		position(xa, za, xb, zb);
}    


//---------------------------------------------------------------------------
// barrier::position
//---------------------------------------------------------------------------
void barrier::position(float xa, float za, float xb, float zb)
{
	if (fVertices != NULL)
	{
		float x1;
		float x2;
		float z1;
		float z2;
		
		fVertices[ 0] = xa; fVertices[ 1] = 0.; 			fVertices[ 2] = za;
		fVertices[ 3] = xa; fVertices[ 4] = gBarrierHeight; fVertices[ 5] = za;
		fVertices[ 6] = xb; fVertices[ 7] = gBarrierHeight; fVertices[ 8] = zb;
		fVertices[ 9] = xb; fVertices[10] = 0.; 			fVertices[11] = zb;
		
		if (xa < xb)
		{
			xmn = x1 = xa;
			xmx = x2 = xb;
			z1 = za;
			z2 = zb;
		}
		else
		{
			xmn = x1 = xb;
			xmx = x2 = xa;
			z1 = zb;
			z2 = za;
		}
		
		if (za < zb)
		{
			zmn = za;
			zmx = zb;
		}
		else
		{
			zmn = zb;
			zmx = za;
		}
		
		a = z2 - z1;
		b = x1 - x2;
		c = x2 * z1  -  x1 * z2;
		f = 1. / sqrt(a*a + b*b);
		sna = -b * f;
		
		if (a < 0.)
			sna *= -1.;
			
		csa =  abs((int)(a * f));
	}
}


#pragma mark -

#if 0
//===========================================================================
// CompareMinX (predicate)
//===========================================================================
struct CompareMinX
{
	bool operator() (barrier* item1, barrier* item2) const
		{
			return (item1->xmin() < item2->xmin());
		}
};
#endif

//---------------------------------------------------------------------------
// TSortedBarrierList::Add
//---------------------------------------------------------------------------
void bxsortedlist::add(barrier* newBarrier)
{
    Q_CHECK_PTR(newBarrier);
    
    bool inserted = false;
	barrier* oldBarrier;

	reset();
	while (next(oldBarrier))
	{
		if (newBarrier->xmin() < oldBarrier->xmin())
        {
			inserthere(newBarrier);
            inserted = true;
            break;
        }
    }
        
    if (!inserted)
		append(newBarrier);
}


//---------------------------------------------------------------------------
// TSortedBarrierList::Sort
//---------------------------------------------------------------------------
void bxsortedlist::sort()
{
	// This technique assumes that the list is almost entirely sorted at the start
	// Hopefully, with some reasonable frame-to-frame coherency, this will be true!
	// Actually, barriers are expected to be static.
    gdlink<barrier*> *savecurr;
    barrier* o = NULL;
    barrier* p = NULL;
    barrier* b = NULL;
    this->reset();
    this->next(p);
    savecurr = currItem;
    while (this->next(o))
    {
        if (o->xmin() < p->xmin())
        {
            gdlink<barrier*>* link = this->unlink();  // at o, just unlink it
            currItem = savecurr;  // back up to previous one directly
            while (this->prev(b)) // then use iterator to move back again
                if (b->xmin() < o->xmin())
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


