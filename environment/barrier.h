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
    bxsortedlist() { }
    ~bxsortedlist() { }
    void add(barrier* a);
    void sort();
};

//===========================================================================
// barrier
//===========================================================================
class barrier : public gpoly
{
public:
	static float gBarrierHeight;
	static Color gBarrierColor;
	static bxsortedlist barrier::gXSortedBarriers;	

    barrier();
    barrier(float xa, float za, float xb, float zb);
    ~barrier();
    
    void position(float xa, float za, float xb, float zb);

    float xmin();
    float xmax();
    float zmin();
    float zmax();
    float dist(float x, float z);
    float sina();
    float cosa();
    
protected:
    void init(float xa, float za, float xb, float zb);

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


inline float barrier::xmin() { return xmn; }
inline float barrier::xmax() { return xmx; }
inline float barrier::zmin() { return zmn; }
inline float barrier::zmax() { return zmx; }
inline float barrier::dist(float x, float z) { return (a*x + b*z + c) * f; }
inline float barrier::sina() { return sna; }
inline float barrier::cosa() { return csa; }


#endif



