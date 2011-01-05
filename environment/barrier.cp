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
#include "datalib.h"
#include "globals.h"
#include "graphics.h"

// barrier globals
bxsortedlist barrier::gXSortedBarriers;
float barrier::gBarrierHeight;
Color barrier::gBarrierColor;

//===========================================================================
// KeyFrame
//===========================================================================
KeyFrame::KeyFrame()
{
	condition = NULL;
}

KeyFrame::~KeyFrame()
{
	if( condition )
		delete condition;
}

//===========================================================================
// TimeCondition
//===========================================================================
KeyFrame::TimeCondition::TimeCondition( long step )
{
	end = step;
}

KeyFrame::TimeCondition::~TimeCondition()
{
}

bool KeyFrame::TimeCondition::isSatisfied( long step )
{
	return (step < end)
		|| ( (end == 0) && (step <= 1) );
}

void KeyFrame::TimeCondition::setActive( long step, bool active )
{
	// no-op
}

long KeyFrame::TimeCondition::getEnd( long step )
{
	return end;
}

//===========================================================================
// CountCondition
//===========================================================================
KeyFrame::CountCondition::CountCondition( const int *count,
										  Op op,
										  int threshold,
										  long duration )
{
	parms.count = count;
	parms.op = op;
	parms.threshold = threshold;
	parms.duration = duration;

	state.end = -1;
}

KeyFrame::CountCondition::~CountCondition()
{
}

bool KeyFrame::CountCondition::isSatisfied( long step )
{
	int count = *(parms.count);
	int threshold = parms.threshold;

	switch( parms.op )
	{
	case EQ:
		return count == threshold;
	case GT:
		return count > threshold;
	case LT:
		return count < threshold;
	default:
		assert( false );
	}
}

void KeyFrame::CountCondition::setActive( long step, bool active )
{
	if( active )
	{
		state.end = step + parms.duration;
	}
	else
	{
		state.end = -1;
	}
}

long KeyFrame::CountCondition::getEnd( long step )
{
	assert( state.end > -1 );

	return state.end;
}

//===========================================================================
// barrier
//===========================================================================

//---------------------------------------------------------------------------
// barrier::barrier
//---------------------------------------------------------------------------
barrier::barrier( int id, int keyFrameCount, bool recordPosition )
{
	this->id = id;
	numKeyFrames = keyFrameCount;
	this->recordPosition = recordPosition;


	numKeyFramesSet = 0;
	activeKeyFrame = NULL;
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

	positionWriter = NULL;

	fNumPoints = 4;
	setcolor(gBarrierColor);

	fVertices = new float[12];  
	if( !fVertices )
		error( 1, "Insufficient memory to setup barrier vertices" );

	xSortNeeded = false;

	position( 0, 0, 0, 0 );
}


//---------------------------------------------------------------------------
// barrier::~barrier
//---------------------------------------------------------------------------
barrier::~barrier()
{
	if( keyFrame )
		delete [] keyFrame;
	
	if( fVertices )
		delete [] fVertices;

	if( positionWriter )
		delete positionWriter;
}


//---------------------------------------------------------------------------
// barrier::setKeyFrame
//---------------------------------------------------------------------------
void barrier::setKeyFrame( KeyFrame::Condition *condition, float xa, float za, float xb, float zb )
{
	if( keyFrame )
	{
		if( numKeyFramesSet < numKeyFrames )
		{
			// TODO: validate time sequence
			keyFrame[numKeyFramesSet].condition = condition;
			keyFrame[numKeyFramesSet].xa   = xa;
			keyFrame[numKeyFramesSet].za   = za;
			keyFrame[numKeyFramesSet].xb   = xb;
			keyFrame[numKeyFramesSet].zb   = zb;

			numKeyFramesSet++;
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
	// determine the active key frame
	{
		KeyFrame *nextKeyFrame = NULL;
		for( int i = 0; i < numKeyFrames; i++ )
		{
			if( keyFrame[i].condition->isSatisfied( step ) )
			{
				nextKeyFrame = &keyFrame[i];
				//cout << "activeframe = " << i << endl;
				break;
			}
		}

		if( nextKeyFrame != activeKeyFrame )
		{
			if( activeKeyFrame )
				activeKeyFrame->condition->setActive( step, false );

			activeKeyFrame = nextKeyFrame;
			if( activeKeyFrame )
				activeKeyFrame->condition->setActive( step, true );
		}
	}

	if( activeKeyFrame )
	{
		KeyFrame *kf = activeKeyFrame;
		long time = max( 1l, kf->condition->getEnd(step) - step  );
		float dxa = (kf->xa - xa) / time;
		float dxb = (kf->xb - xb) / time;
		float dza = (kf->za - za) / time;
		float dzb = (kf->zb - zb) / time;

		//cout << "DELTA = (" << dxa << "," << dza << "," << dxb << "," << dzb << ")" << endl;

		if( (dxa != 0.0) || (dxb != 0.0) || (dza != 0.0) || (dzb != 0.0) )
		{
			position( xa + dxa, za + dza, xb + dxb, zb + dzb );

			if( recordPosition )
			{
				if( positionWriter == NULL )
				{
					char path[512];
					sprintf( path,
							 "run/motion/position/barriers/position_%d.txt",
							 id );

					positionWriter = new DataLibWriter( path );

					const char *colnames[] = {"Time", "X1", "Z1", "X2","Z2", NULL};
					const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT};
					positionWriter->beginTable( "Positions",
												colnames,
												coltypes );		
				}

				positionWriter->addRow( step,
										xa, za, xb, zb );
			}
		}
	}
}


//---------------------------------------------------------------------------
// barrier::position
//---------------------------------------------------------------------------
void barrier::position( float xa, float za, float xb, float zb )
{
//	printf( "barrier::position( xa=%g, za=%g, xb=%g, zb=%g )\n", xa, za, xb, zb );

	this->xa = xa;
	this->za = za;
	this->xb = xb;
	this->zb = zb;

	//cout << "(" << xa << "," << za << "," << xb << "," << zb << ")" << endl;
	
	if( fVertices != NULL )
	{
		float x1;
		float z1;
		float x2;
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
			
		csa =  fabs(a * f);
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


