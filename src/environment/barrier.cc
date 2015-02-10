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
float barrier::gBarrierHeight;
Color barrier::gBarrierColor;
bool barrier::gStickyBarriers;
bool barrier::gRatioPositions;
bxsortedlist barrier::gXSortedBarriers;
vector<barrier *> barrier::gBarriers;


//===========================================================================
// barrier
//===========================================================================

//---------------------------------------------------------------------------
// barrier::barrier
//---------------------------------------------------------------------------
barrier::barrier()
: nextPosition(0,0,0,0)
, currPosition(0,0,0,0)
, absCurrPosition(0,0,0,0)
{
	fNumPoints = 4;

	fVertices = new float[12];  
	if( !fVertices )
		error( 1, "Insufficient memory to setup barrier vertices" );
}


//---------------------------------------------------------------------------
// barrier::~barrier
//---------------------------------------------------------------------------
barrier::~barrier()
{
	if( fVertices )
		delete [] fVertices;
}

//---------------------------------------------------------------------------
// barrier::init
//---------------------------------------------------------------------------
void barrier::init()
{
	setcolor(gBarrierColor);
	updateVertices();
}

//---------------------------------------------------------------------------
// barrier::update
//---------------------------------------------------------------------------
void barrier::update()
{
	if( nextPosition != currPosition )
		updateVertices();
}


//---------------------------------------------------------------------------
// barrier::updateVertices
//---------------------------------------------------------------------------
void barrier::updateVertices()
{
	currPosition = nextPosition;
	absCurrPosition = currPosition;
	if( gRatioPositions )
	{
		absCurrPosition.xa *= globals::worldsize;
		absCurrPosition.za *= globals::worldsize;
		absCurrPosition.xb *= globals::worldsize;
		absCurrPosition.zb *= globals::worldsize;
	}

	float x1;
	float z1;
	float x2;
	float z2;

	fVertices[ 0] = absCurrPosition.xa; fVertices[ 1] = 0.; 			 fVertices[ 2] = absCurrPosition.za;
	fVertices[ 3] = absCurrPosition.xa; fVertices[ 4] = gBarrierHeight; fVertices[ 5] = absCurrPosition.za;
	fVertices[ 6] = absCurrPosition.xb; fVertices[ 7] = gBarrierHeight; fVertices[ 8] = absCurrPosition.zb;
	fVertices[ 9] = absCurrPosition.xb; fVertices[10] = 0.; 			 fVertices[11] = absCurrPosition.zb;
		
	if( absCurrPosition.xa < absCurrPosition.xb )
	{
		xmn = x1 = absCurrPosition.xa;
		xmx = x2 = absCurrPosition.xb;
		z1 = absCurrPosition.za;
		z2 = absCurrPosition.zb;
	}
	else
	{
		xmn = x1 = absCurrPosition.xb;
		xmx = x2 = absCurrPosition.xa;
		z1 = absCurrPosition.zb;
		z2 = absCurrPosition.za;
	}
		
	if( absCurrPosition.za < absCurrPosition.zb )
	{
		zmn = absCurrPosition.za;
		zmx = absCurrPosition.zb;
	}
	else
	{
		zmn = absCurrPosition.zb;
		zmx = absCurrPosition.za;
	}
		
	a = z2 - z1;
	b = x1 - x2;
	c = x2 * z1  -  x1 * z2;
	if( (a == 0.0) && (b == 0.0) )
	{
		// zero-size barrier, so distance is meaningless; make it very large
		c = 1.0;
		f = 1.0e+10;
	}
	else
		f = 1. / sqrt( a*a + b*b );
	sna = -b * f;
		
	if( a < 0. )
		sna *= -1.;
			
	csa =  fabs(a * f);	
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
	
	// If any barrier says it needs to be x-sorted, then they all must be
	if( !needXSort )
		needXSort = newBarrier->needXSort();
}


//---------------------------------------------------------------------------
// TSortedBarrierList::actuallyXSort
//---------------------------------------------------------------------------
void bxsortedlist::actuallyXSort()
{
	// This technique assumes that the list is almost entirely sorted at the start
	// Hopefully, with some reasonable frame-to-frame coherency, this will be true!
	// Actually, barriers are expected to be static.  Well, they used to be until
	// I added keyFrames, so they could be dynamic...
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


