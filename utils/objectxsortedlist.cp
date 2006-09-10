/*---------------------------------------------------------------------------
 *
 * objectxsortedlist.cp
 *
 * A list to store all types of polyworld objects
 *
 -------------------------------------------------------------------------*/



#include "objectxsortedlist.h"
#include "critter.h"

#define DebugCounts 0

#if DebugCounts
	#define cntPrint( x... ) printf( x )
#else
	#define cntPrint( x... )
#endif

using namespace std;


class objectxsortedlist;


// Globals 

// The big list of all objects (critters, food, bricks, etc.)
objectxsortedlist objectxsortedlist::gXSortedObjects;


//---------------------------------------------------------------------------
// objectxsortedlist::getCount
//---------------------------------------------------------------------------
int objectxsortedlist::getCount( int objType )
{
    /* Update the counts based on which type of object was added to the list */
    switch( objType )
	{
		case CRITTERTYPE:
			return critterCount;
			break;
		case FOODTYPE:
			return foodCount;
			break;
		case BRICKTYPE:
			return brickCount;
			break;
    }
    
    // Error!  Should not get here.
    printf( "%s: ERROR--Unknown object type\n", __func__ );
    exit( 1 );
}



//---------------------------------------------------------------------------
// objectxsortedlist::lastObj
//---------------------------------------------------------------------------
int objectxsortedlist::lastObj( int objType, gobject** g )
{
    int returnVal;

    // Start with the last object
    returnVal = this->last( *g );
    
    // Traverse the list backwards to find the last object of the given type
    while( (*g)->getType() != objType )
		returnVal = this->prev(*g);
    
    // Return whatever prev or last returned
    return returnVal;
}



//---------------------------------------------------------------------------
// objectxsortedlist::nextObj
//---------------------------------------------------------------------------
// Get the next object of a given type
//
int objectxsortedlist::nextObj( int objType, gobject** g )
{
    int returnVal;

    // Start with the next object in the list
    returnVal = this->next( *g );
	if( !returnVal )
		return returnVal;

    // Traverse the list forwards looking for an appropriate object
    while( (*g)->getType() != objType )
	{
		returnVal = this->next( *g );
		if( returnVal == 0 )
			return returnVal;
    }

    return returnVal;
}


//---------------------------------------------------------------------------
// objectxsortedlist::prevObj
//---------------------------------------------------------------------------
// Get the previous object of a given type
int objectxsortedlist::prevObj( int objType, gobject** g )
{
    int returnVal;
	
    // Start with the previous object
    returnVal = this->prev( *g );
	if( !returnVal )
		return( returnVal );

    // Traverse backwards looking for an object of the given type
    while( (*g)->getType() != objType )
	{
		returnVal = this->prev( *g );
		if( !returnVal )
			return returnVal;
    }
	
    return returnVal;
}


//---------------------------------------------------------------------------
// objectxsortedlist::add
//---------------------------------------------------------------------------
// Add an object to the all objects list
void objectxsortedlist::add( gobject* a )
{
#ifdef DEBUGCALLS
    pushproc( "objectxsortedlist::add" );
#endif DEBUGCALLS
    bool inserted = false;
    gobject* o;
    this->reset();
    while( this->next( o ) )
	{
        if( (a->x() - a->radius()) < (o->x() - o->radius()) )
		{
			a->listLink = this->inserthere(a);
            inserted = true;
            break;
        }
    }

    if( !inserted )
		a->listLink = this->append( a );
    
    // Increase object type count based on added object's type
    switch( a->getType() )
	{
		case CRITTERTYPE:
			cntPrint( "%s: incrementing critterCount from %d to %d\n", __func__, critterCount, critterCount + 1 );
			critterCount++;
			break;
		case FOODTYPE:
			cntPrint( "%s: incrementing foodCount from %d to %d\n", __func__, foodCount, foodCount + 1 );
			foodCount++;
			break;
		case BRICKTYPE:
			cntPrint( "%s: incrementing brickCount from %d to %d\n", __func__, brickCount, brickCount + 1 );
			brickCount++;
			break;
		default:
			fprintf( stderr, "%s() called for x-sorted object list with invalid object type (%d)\n", __func__, a->getType() );
			break;
    }

#ifdef DEBUGCALLS
    popproc();
#endif DEBUGCALLS

}


//---------------------------------------------------------------------------
// objectxsortedlist::removeCurrentObject
//---------------------------------------------------------------------------
// Remove the current object and decrement the appropriate count
void objectxsortedlist::removeCurrentObject()
{
    int validReturn;
    gobject* o;

    // Start with the previous object
    validReturn = this->current( o );

	if( validReturn )
	{
		// decrease object type count based on added object's type
		// and deal with the case when the marked item is what is being removed
		switch( o->getType() )
		{
			gdlink<gobject*> *tempItem;

			case CRITTERTYPE:
				cntPrint( "%s: decrementing critterCount from %d to %d\n", __func__, critterCount, critterCount - 1 );
				critterCount--;
				tempItem = currItem;
				while( (markedCritter == currItem) && (markedCritter != 0) )
				{
					if( tempItem == lastItem->nextItem )	// backed up to the head of the list
						markedCritter = 0;					// so there won't be a mark for this type anymore
					else
					{
						tempItem = tempItem->prevItem;				// back up one item in the list
						if( tempItem->e->getType() == CRITTERTYPE )	// if it's of the right type,
							markedCritter = tempItem;				// mark it (and that will kick us out of the loop)
					}
				}
				break;

			case FOODTYPE:
				cntPrint( "%s: decrementing foodCount from %d to %d\n", __func__, foodCount, foodCount - 1 );
				foodCount--;
				tempItem = currItem;
				while( (markedFood == currItem) && (markedFood != 0) )
				{
					if( tempItem == lastItem->nextItem )	// backed up to the head of the list
						markedFood = 0;						// so there won't be a mark for this type anymore
					else
					{
						tempItem = tempItem->prevItem;				// back up one item in the list
						if( tempItem->e->getType() == FOODTYPE )	// if it's of the right type,
							markedFood = tempItem;					// mark it (and that will kick us out of the loop)
					}
				}
				break;

			case BRICKTYPE:
				cntPrint( "%s: decrementing brickCount from %d to %d\n", __func__, brickCount, brickCount - 1 );
				brickCount--;
				tempItem = currItem;
				while( (markedBrick == currItem) && (markedBrick != 0) )
				{
					if( tempItem == lastItem->nextItem )	// backed up to the head of the list
						markedBrick = 0;					// so there won't be a mark for this type anymore
					else
					{
						tempItem = tempItem->prevItem;				// back up one item in the list
						if( tempItem->e->getType() == BRICKTYPE )	// if it's of the right type,
							markedBrick = tempItem;					// mark it (and that will kick us out of the loop)
					}
				}
				break;

			default:
				printf( "%s: object in list has invalid type (%d)\n", __func__, o->getType() );
				break;
		}

		// Actually remove the object from the list
		this->remove();
		
		if( kount != (critterCount + foodCount + brickCount) )
		{
			printf( "kount (%ld) != critterCount (%d) + foodCount (%d) + brickCount (%d)\n", kount, critterCount, foodCount, brickCount );
		}
    }
}


//---------------------------------------------------------------------------
// objectxsortedlist::removeObjectWithLink
//---------------------------------------------------------------------------
// Remove the provided object and decrement the appropriate count
// The provided object MUST have a valid ListLink
void objectxsortedlist::removeObjectWithLink( gobject* o )
{
	gdlink<gobject*> *item = o->GetListLink();
	gdlink<gobject*> *saveCurr;
	int countType;
	
	saveCurr = 0;
	if( currItem != item )
		saveCurr = currItem;
	currItem = item;
	removeCurrentObject();	// will take care of currItem, markItem, and marked<type> if they point to the item being removed
    switch( o->getType() )
	{
		case CRITTERTYPE:
			countType = critterCount;
			break;
		case FOODTYPE:
			countType = foodCount;
			break;
		case BRICKTYPE:
			countType = brickCount;
			break;
		default:
			printf( "ERROR: tried to remove object with link using invalid object type (%d)\n", o->getType() );
			countType = kount;	// not very meaningful, but nothing is at this point
			break;
    }

	if( countType && saveCurr )
		currItem = saveCurr;
	return;
}


//---------------------------------------------------------------------------
// objectxsortedlist::sort
//---------------------------------------------------------------------------
void objectxsortedlist::sort()
{

#ifdef DEBUGCALLS
    pushproc("objectxsortedlist::sort");
#endif DEBUGCALLS
    // This technique assumes that the list is almost entirely sorted at the start
    // Hopefully, with some reasonable frame-to-frame coherency, this will be true!
    gdlink<gobject*> *savecurr;
    gobject* o = NULL;
    gobject* p = NULL;
    gobject* b = NULL;
    this->reset();
    this->next( p );
    savecurr = currItem;
    while( this->next( o ) )
	{
		if( (o->x() - o->radius()) < (p->x() - p->radius()) )
		{
			gdlink<gobject*>* link = this->unlink();  // at o, just unlink
			currItem = savecurr;  // back up to previous one directly
			while( this->prev( b ) ) // then use iterator to move back again
				if( (b->x() - b->radius()) < (o->x() - o->radius()) )
					break; // until we have one that starts before o
			if( currItem )  // we did find one, and didn't run off beginning of list
				this->appendhere( link );
			else  // we have a new head of the list
				this->insert( link );
			currItem = savecurr;
			o =	p;
		}
		p = o;
		savecurr = currItem;
    }
#ifdef DEBUGCALLS
    popproc();
#endif DEBUGCALLS
}


//---------------------------------------------------------------------------
// objectxsortedlist::list
//---------------------------------------------------------------------------
void objectxsortedlist::list()
{
#ifdef DEBUGCALLS
    pushproc("objectxsortedlist::list");
#endif DEBUGCALLS
    gdlink<gobject*> *savecurr;
    gobject* pobj;
    savecurr = currItem;
    cout << "c" eql currItem sp "m" eql markItem sp "l" eql lastItem sp "f" eql lastItem->nextItem << " ";
    this->reset();
    cout << this->kount << ":";
    while( this->next( pobj ) )
	{
		if( pobj->getType() == CRITTERTYPE )
		{
			critter* c;
			c = (critter*) pobj;
			//cout sp c->Number() << "/" << pobj << "/" << pobj->GetListLink();
			cout sp c->Number() << "(X=" << c->X() << ")";
		}
    }
    cout nlf;
    currItem = savecurr;
#ifdef DEBUGCALLS
    popproc();
#endif DEBUGCALLS
}





/* Mark functions updated for multiple object types */

/* 
   Set a mark on the current object (if it is correct type)
   If it is not the correct type, then fail and exit
*/
void objectxsortedlist::setMark( int objType )
{
    gdlink<gobject*>* lookAt = currItem;
    if( lookAt->e->getType() != objType )
	{
		printf( "ERROR: mark objtype (%d) != current objtype (%d)\n", objType, lookAt->e->getType() );
		exit( 1 );
    }

    /*  Other option is to search forward to find object of correct type
	while( lookAt->e->getType() != objType )
	{
		lookAt = lookAt->nextItem;
		if( lookAt == lastItem )
		{
			// Went to the end of the list
			printf("Setting mark past the end of the list.\n");
			//exit(1);
		}
    }
    */

    if( objType == CRITTERTYPE )
		markedCritter = lookAt;
    else if( objType == FOODTYPE )
		markedFood = lookAt;
    else if( objType == BRICKTYPE )
		markedBrick = lookAt;
    else
		printf( "ERROR: Trying to set mark with illegal type (%d)\n", objType );
}


void objectxsortedlist::setMarkPrevious( int objType )
{
    gdlink<gobject*>* lookAt = currItem->prevItem;
    while( lookAt->e->getType() != objType )
	{
		lookAt = lookAt->prevItem;
		if( lookAt == lastItem )
		{
			// Went to the beginning of the list
			//printf( "Setting mark past the beginning of the list.\n" );
			//exit( 1 );
		}
    }

    if( objType == CRITTERTYPE )
		markedCritter = lookAt;
    else if( objType == FOODTYPE )
		markedFood = lookAt;
    else if( objType == BRICKTYPE )
		markedBrick = lookAt;
    else
		printf( "ERROR: Trying to set mark to prev item with illegal type (%d)\n", objType );
}


/*
  Try to set the mark on the last item.  If it is the wrong object type,
  then set the mark on the last item of allowable type.
*/
void objectxsortedlist::setMarkLast( int objType )
{
    gdlink<gobject*>* lookAt = lastItem;
    while( lookAt->e->getType() != objType )
	{
		lookAt = lookAt->prevItem;
		if( lookAt == lastItem )
		{
			// Went to the end of the list
			//printf( "Setting mark past the end of the list.\n" );
			//exit( 1 );
		}
    }
	
    if( objType == CRITTERTYPE )
		markedCritter = lookAt;
    else if( objType == FOODTYPE )
	    markedFood = lookAt;
    else if( objType == BRICKTYPE )
		markedBrick = lookAt;
    else
		printf( "ERROR: Trying to set mark to last item with illegal type (%d)\n", objType );
}

void objectxsortedlist::toMark( int objType )
{
    if (objType == CRITTERTYPE)
		currItem = markedCritter;
    else if (objType == FOODTYPE)
		currItem = markedFood;
    else if (objType == BRICKTYPE)
		currItem = markedBrick;
	else
		printf( "ERROR: Trying to go to mark for illegal type (%d)\n", objType );
}

void objectxsortedlist::getMark( int objType, gobject* gob )
{
    if( objType == CRITTERTYPE )
	{
		if( markedCritter )
			gob = markedCritter->e;
		else
			gob = 0;
	}
	else if( objType == FOODTYPE )
	{
		if( markedFood )
			gob = markedFood->e;
		else
			gob = 0;
	}
	else if (objType == BRICKTYPE)
	{
		if (markedBrick)
			gob = markedBrick->e;
		else
			gob = 0;
    }
	else
		printf( "ERROR: Trying to get mark for illegal type (%d)\n", objType );
}



