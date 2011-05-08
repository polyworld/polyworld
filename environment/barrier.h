/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// barrier.h - declaration of barrier classes

#ifndef BARRIER_H
#define BARRIER_H

// STL
//#include <list>

// Local
#include "condprop.h"
#include "gdlink.h"
#include "gpolygon.h"
#include "objectlist.h"

// Forward declarations
class barrier;


//===========================================================================
// TSortedBarrierList
//===========================================================================

class bxsortedlist : public gdlist<barrier*>
{
public:
    bxsortedlist() { needXSort = false; }
    ~bxsortedlist() { }
    void add(barrier* a);
    void xsort();
private:
	bool needXSort;
	void actuallyXSort();
};

inline void bxsortedlist::xsort() { if( needXSort ) actuallyXSort(); }


//===========================================================================
// barrier
//===========================================================================
class barrier : public gpoly
{
public:
	static float gBarrierHeight;
	static Color gBarrierColor;
	static bool gStickyBarriers;
	static bxsortedlist gXSortedBarriers;	

    barrier();
    ~barrier();
    
	void update();
	
	condprop::LineSegment *getPosition();

    float xmin();
    float xmax();
    float zmin();
    float zmax();
    float dist( float x, float z );
    float sina();
    float cosa();
	
	bool needXSort();
    
protected:
    void updateVertices();

	condprop::LineSegment position;
	condprop::LineSegment verticesPosition;

    float xmn;
    float xmx;
    float zmn;
    float zmx;
    float a;
    float b;
    float c;
    float f;
    float sna;
    float csa;
};

inline condprop::LineSegment *barrier::getPosition() { return &position; }
inline float barrier::xmin() { return xmn; }
inline float barrier::xmax() { return xmx; }
inline float barrier::zmin() { return zmn; }
inline float barrier::zmax() { return zmx; }
inline float barrier::sina() { return sna; }
inline float barrier::cosa() { return csa; }
inline float barrier::dist( float x, float z ) { return (a*x + b*z + c) * f; }
inline bool  barrier::needXSort() { return false; }


#endif



