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
barrier::barrier( int keyFrameCount )
{
	numKeyFrames = keyFrameCount;
	numKeyFramesSet = 0;
	currentKeyFrame = 0;
	if( numKeyFrames > 0 )
	{
		keyFrame = new KeyFrame[numKeyFrames];
		if( !keyFrame )
			error( 1, "Insufficient memory to setup barrier keyFrames" );
	}
	else
	{
		error( 1, "Invalid number of barrier keyFrames (", numKeyFrames, ")" );
		keyFrame = NULL;
	}

	fNumPoints = 4;
	setcolor(gBarrierColor);

	fVertices = new float[12];  
	if( !fVertices )
		error( 1, "Insufficient memory to setup barrier vertices" );

	xSortNeeded = false;
}


//---------------------------------------------------------------------------
// barrier::~barrier
//---------------------------------------------------------------------------
barrier::~barrier()
{
	if( keyFrame )
		delete keyFrame;
	
	if( fVertices )
		delete fVertices;
}


//---------------------------------------------------------------------------
// barrier::setKeyFrame
//---------------------------------------------------------------------------
void barrier::setKeyFrame( long step, float xa, float za, float xb, float zb )
{
	if( keyFrame )
	{
		if( numKeyFramesSet < numKeyFrames )
		{
			// If it's the first keyframe or time is correctly increasing monotonically...
			if( (numKeyFramesSet == 0) || (step > keyFrame[numKeyFramesSet-1].step) )
			{
				keyFrame[numKeyFramesSet].step = step;
				keyFrame[numKeyFramesSet].xa   = xa;
				keyFrame[numKeyFramesSet].za   = za;
				keyFrame[numKeyFramesSet].xb   = xb;
				keyFrame[numKeyFramesSet].zb   = zb;
				
				if( numKeyFramesSet == 0 )	// first one
				{
					position( xa, za, xb, zb );	// initialize the barrier's position
				}
				else if( !xSortNeeded )
				{
					if( (keyFrame[numKeyFramesSet].xa != keyFrame[numKeyFramesSet-1].xa) ||
						(keyFrame[numKeyFramesSet].xb != keyFrame[numKeyFramesSet-1].xb) )
						xSortNeeded = true;
				}

				numKeyFramesSet++;
			}
			else
				error( 1, "Barrier keyframes specified out of temporal sequence" );
		}
		else
			error( 1, "More barrier keyframes specified than requested for allocation" );
	}
}


//---------------------------------------------------------------------------
// barrier::update
//---------------------------------------------------------------------------
void barrier::update( long step )
{
	// If the barrier is static or something went wrong allocating the keyframes
	// or we've stepped beyond the final keyframe, then there's nothing to do
	if( (numKeyFrames <= 1) || !keyFrame || (step > keyFrame[numKeyFramesSet-1].step) )
		return;

	// Note:  Since we guaranteed that the keyframe time steps were specified
	// in monotonically increasing order in setKeyFrame() and we only reach here
	// if we are not past the end of the last keyFrame, and we only update
	// currentKeyFrame after we're done processing an update for a given step,
	// there should always be a *next* keyframe available for us to test against.

	// If we've reached the next key frame, have to do some special updating
	if( step == keyFrame[currentKeyFrame+1].step )
	{
		int i = ++currentKeyFrame;
		
		position( keyFrame[i].xa, keyFrame[i].za, keyFrame[i].xb, keyFrame[i].zb );
	}
	else
	{
		int i = currentKeyFrame;
		int j = i + 1;
		
		// If there's any change in x or z from one keyframe to the next...
		if( (keyFrame[i].xa != keyFrame[j].xa) || (keyFrame[i].za != keyFrame[j].za) ||
			(keyFrame[i].xb != keyFrame[j].xb) || (keyFrame[i].zb != keyFrame[j].zb) )
		{
			float fraction = (float) (step - keyFrame[i].step) / (keyFrame[j].step - keyFrame[i].step);
			float xa = keyFrame[i].xa  +  fraction * (keyFrame[j].xa - keyFrame[i].xa);
			float xb = keyFrame[i].xb  +  fraction * (keyFrame[j].xb - keyFrame[i].xb);
			float za = keyFrame[i].za  +  fraction * (keyFrame[j].za - keyFrame[i].za);
			float zb = keyFrame[i].zb  +  fraction * (keyFrame[j].zb - keyFrame[i].zb);
			position( xa, za, xb, zb );
		}
	}
}


//---------------------------------------------------------------------------
// barrier::position
//---------------------------------------------------------------------------
void barrier::position( float xa, float za, float xb, float zb )
{
//	printf( "barrier::position( xa=%g, za=%g, xb=%g, zb=%g )\n", xa, za, xb, zb );
	
	if( fVertices != NULL )
	{
		float x1;
		float x2;
		float z1;
		float z2;
		
		fVertices[ 0] = xa; fVertices[ 1] = 0.; 			fVertices[ 2] = za;
		fVertices[ 3] = xa; fVertices[ 4] = gBarrierHeight; fVertices[ 5] = za;
		fVertices[ 6] = xb; fVertices[ 7] = gBarrierHeight; fVertices[ 8] = zb;
		fVertices[ 9] = xb; fVertices[10] = 0.; 			fVertices[11] = zb;
		
		if( xa < xb )
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
		
		if( za < zb )
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
			
		csa =  abs( (int) (a * f) );
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


