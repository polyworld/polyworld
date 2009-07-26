#ifndef GPOLYGON_H
#define GPOLYGON_H

// System
#include <gl.h>

// Local
#include "gobject.h"


//===========================================================================
// gpoly
//===========================================================================
struct opoly // Object POLY's for gpolyobj objects (don't need gobject members)
{
    long fNumPoints;
    float* fVertices;
};

class gpoly : public gobject
{

friend std::istream& operator>>(std::istream&, gpoly&);

public:
    gpoly();
    gpoly(char* n);
    gpoly(long np,float* v);
    gpoly(char* n, long np,float* v);
    gpoly(const opoly& p);
    gpoly(char* n, const opoly& p);
    gpoly(const opoly* pp);
    gpoly(char* n, const opoly* pp);

    
    void setradius(float r);
    void setradiusscale(float s);
    void setscale(float s);
    
    float lx();
    float ly();
    float lz();
    float radiusscale();
    
    virtual void draw();
    virtual void print();
        
protected:
    virtual void setradius();
    void init();

    long fNumPoints;
    float* fVertices;
    float fLength[3];
    float fRadiusScale;
    bool fRadiusFixed;
};

inline float gpoly::lx() { return fLength[0]; }
inline float gpoly::ly() { return fLength[1]; }
inline float gpoly::lz() { return fLength[2]; }
inline float gpoly::radiusscale() { return fRadiusScale; }



//===========================================================================
// gpolyobj
//===========================================================================
class gpolyobj : public gobject
{
friend void operator >> (const char*, gpolyobj&);

public:
    gpolyobj();
    gpolyobj(long np, opoly* p);
    gpolyobj(char* n, long np,opoly* p);
    ~gpolyobj();
    
    void clonegeom(const gpolyobj& o);

    void setradius(float r);
    void setradiusscale(float s);
    void setscale(float s);

    float lx();
    float ly();
    float lz();
    float radiusscale();
	long numPolygons();

    void drawcolpolyrange(long i1, long i2, float* color);
    
    virtual void draw();
    virtual void print();

    
protected:    
    virtual void setradius();
    void setlen();
    void init(long np, opoly* p);

    long fNumPolygons;
    opoly* fPolygon;
    float fLength[3];
    float fRadiusScale;
    bool fRadiusFixed;    
};

inline float gpolyobj::lx() { return fLength[0]; }
inline float gpolyobj::ly() { return fLength[1]; }
inline float gpolyobj::lz() { return fLength[2]; }
inline float gpolyobj::radiusscale() { return fRadiusScale; }
inline long  gpolyobj::numPolygons() { return fNumPolygons; }

#endif
