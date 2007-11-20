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

struct KeyFrame
{
	long	step;
	float	xa;
	float	za;
	float	xb;
	float	zb;
};
typedef struct KeyFrame KeyFrame;

class barrier : public gpoly
{
public:
	static float gBarrierHeight;
	static Color gBarrierColor;
	static bxsortedlist gXSortedBarriers;	

    barrier( int keyFrameCount );
    ~barrier();
    
	void setKeyFrame( long step, float xa, float za, float xb, float zb );
	
	void init();	// must be called after the first keyframe is set
	void update( long step );
	
    float xmin();
    float xmax();
    float zmin();
    float zmax();
    float dist( float x, float z );
    float sina();
    float cosa();
	
	bool needXSort();
    
protected:
    void position( float xa, float za, float xb, float zb );

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
	
	int	numKeyFrames;
	int numKeyFramesSet;
	int currentKeyFrame;
	KeyFrame* keyFrame;
	
	bool xSortNeeded;
};


inline float barrier::xmin() { return xmn; }
inline float barrier::xmax() { return xmx; }
inline float barrier::zmin() { return zmn; }
inline float barrier::zmax() { return zmx; }
inline float barrier::sina() { return sna; }
inline float barrier::cosa() { return csa; }
inline float barrier::dist( float x, float z ) { return (a*x + b*z + c) * f; }
inline bool  barrier::needXSort() { return xSortNeeded; }


#endif



