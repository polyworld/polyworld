#ifndef QSQUARE_H
#define QSQUARE_H

// Local
#include "gobject.h"

//===========================================================================
// gsquare
//===========================================================================
class gsquare : public gobject
{
public:
    gsquare();
    gsquare(char* n);
    gsquare(float lx, float ly);
    gsquare(char* n, float lx, float ly);
    gsquare(float xa, float ya, float xb, float yb);
    gsquare(char* n, float xa, float ya, float xb, float yb);
        
    void init();

    void setsquare(float lx, float ly);
    void setsquare(float lx, float ly, float x, float y);
    void setrect(float xa, float ya, float xb, float yb);
    void setradius(float r);
    void setradiusscale(float s);
    void setscale(float s);
    
    float getlenx();
    float getleny();
    float getwidth();
    float getheight();
    float lx();
    float ly();
    float radiusscale();
    
    virtual void draw();
    virtual void print();

protected:
	void setradius();

	float fLengthX;
	float fLengthY;
	bool fFilled;
	bool fRadiusFixed;
	float fRadiusScale;
};

inline float gsquare::getlenx() { return fLengthX; }
inline float gsquare::getleny() { return fLengthY; }
inline float gsquare::getwidth() { return fLengthX; }
inline float gsquare::getheight() { return fLengthY; }
inline float gsquare::lx() { return fLengthX; }
inline float gsquare::ly() { return fLengthY; }
inline float gsquare::radiusscale() { return fRadiusScale; }



//===========================================================================
// gsquaref
//===========================================================================
class gsquaref : public gsquare
{
public:
    gsquaref() : gsquare() { fFilled = true; }
    gsquaref(char* n) : gsquare( n ) { fFilled = true; }
    gsquaref(float lx, float ly) : gsquare( lx, ly ) { fFilled = true; }
    gsquaref(char* n, float lx, float ly) : gsquare( n, lx, ly ) { fFilled = true; }
    gsquaref(float xa, float ya, float xb, float yb) : gsquare( xa, ya, xb, yb ) { fFilled = true; }
    gsquaref(char* n, float xa, float ya, float xb, float yb) : gsquare( n, xa, ya, xb, yb ) { fFilled = true; }
};



//===========================================================================
// gbox
//===========================================================================
class gbox : public gobject
{
public:
    gbox() { init(); setsize(1.,1.,1.); }
    gbox(char* n) : gobject(n) { init(); setsize(1.,1.,1.); }
    gbox(float lx, float ly, float lz) { init(); setsize(lx,ly,lz); }
    gbox(char* n, float lx, float ly, float lz) : gobject(n) { init(); setsize(lx,ly,lz); }
    
    void init()
    {
        fRadiusFixed = false;
        fRadiusScale = 1.0;
    }
    
    void setsize(float lx, float ly, float lz)
        { fLength[0] = lx; fLength[1] = ly; fLength[2] = lz; setradius(); }
    void setlen(float lx, float ly, float lz)
        { fLength[0] = lx; fLength[1] = ly; fLength[2] = lz; setradius(); }
    void setlen(int i, float l) { fLength[i] = l; setradius(); }
    void setlenx(float lx) { fLength[0] = lx; setradius(); }
    void setleny(float ly) { fLength[1] = ly; setradius(); }
    void setlenz(float lz) { fLength[2] = lz; setradius(); }
    void setradius(float r) { fRadiusFixed = true; gobject::setradius(r); srPrint( "    gbox::%s(r): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" ); }
    void setradiusscale(float s)
        { fRadiusFixed = false; fRadiusScale = s; setradius(); }
    void setscale(float s) { fScale = s; setradius(); }
    float getlen(int i) { return fLength[i]; }
    float getlenx() { return fLength[0]; }
    float getleny() { return fLength[1]; }
    float getlenz() { return fLength[2]; }
    float lx()   { return fLength[0]; }
    float ly()   { return fLength[1]; }
    float lz()   { return fLength[2]; }
    float radiusscale() { return fRadiusScale; }
    virtual void draw();
    virtual void print();

protected:    
    virtual void setradius();
        
    bool fFilled;
	float fLength[3];
	bool fRadiusFixed;
	float fRadiusScale;
};



//===========================================================================
// gboxf
//===========================================================================
class gboxf : public gbox
{
public:
    gboxf() { fFilled = true; }
    gboxf(float lx, float ly, float lz) : gbox( lx, ly, lz ) { fFilled = true; }
    gboxf(char* n, float lx, float ly, float lz) : gbox( n, lx, ly, lz ) { fFilled = true; }    
};


#endif
