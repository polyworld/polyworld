#ifndef OBJECTXSORTEDLIST_H
#define OBJECTXSORTEDLIST_H

#define NEXT 1
#define PREV 2

#include "gdlink.h"
#include "agent/agent.h"
#include "environment/brick.h"
#include "environment/food.h"
#include "graphics/gobject.h"
#include "proplib/cppprops.h"

using namespace std;


//===========================================================================
// Sorted list of all objects: agents, food, bricks, other.
//===========================================================================

class objectxsortedlist : public gdlist<gobject*>
{
	PROPLIB_CPP_PROPERTIES

 private:
    int agentCount;
    int foodCount;
    int brickCount;
    gdlink<gobject*> *markedAgent;	
    gdlink<gobject*> *markedFood;	
    gdlink<gobject*> *markedBrick;	

 public:
    objectxsortedlist() { markedAgent = 0; markedFood = 0; markedBrick = 0; }
    ~objectxsortedlist() { }
    void add( gobject* a );
    void removeCurrentObject();
	void removeObjectWithLink( gobject* o );
    void sort();
    void list();
    int getCount( int objType );
    int nextObj( int objType, gobject** gob );
    int prevObj( int objType, gobject** gob );
    int lastObj( int objType, gobject** gob );
	int anotherObj( int direction, int objType, gobject** gob );

    void setMark( int objType );
    void setMarkPrevious( int objType );
    void setMarkLast( int objType );
    void toMark( int objType );
    void getMark( int objType, gobject* gob );

    static objectxsortedlist gXSortedObjects;
};

#define xfor( TYPE, VARTYPE, VAR )										\
	objectxsortedlist::gXSortedObjects.reset();							\
	for( VARTYPE *VAR; objectxsortedlist::gXSortedObjects.nextObj(TYPE, (gobject**)&VAR); ) \

#endif
