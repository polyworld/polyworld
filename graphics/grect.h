
#ifndef GRECT_H
#define GRECT_H

// Local
#include "gobject.h"

//===========================================================================
// grect
//===========================================================================
class grect : public gobject
{
public:
    grect();
    grect(char* n);
    grect(float lx, float ly);
    grect(char* n, float lx, float ly);
    grect(float xa, float ya, float xb, float yb);
    grect(char* n, float xa, float ya, float xb, float yb);

    void init();

    void setrect(float lx, float ly);
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
    float fLengthX;
    float fLengthY;
    bool fFilled;
    bool fRadiusFixed;
    float fRadiusScale;
    virtual void setradius()
    {
        if( !fRadiusFixed )  //  only set radius anew if not set manually
            fRadius = sqrt( fLengthX*fLengthX + fLengthY*fLengthY ) * fRadiusScale * fScale * 0.5;
		srPrint( "grect::%s(): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
    }    
};

inline float grect::getlenx()   { return fLengthX; }
inline float grect::getleny()   { return fLengthY; }
inline float grect::getwidth()  { return fLengthX; }
inline float grect::getheight() { return fLengthY; }
inline float grect::lx()   { return fLengthX; }
inline float grect::ly()   { return fLengthY; }
inline float grect::radiusscale() { return fRadiusScale; }




//===========================================================================
// grectf
//===========================================================================
class grectf : public grect
{
public:
    grectf() { fFilled = true; }
    grectf(char* n) : grect(n) { fFilled = true; }
    grectf(float lx, float ly) : grect(lx,ly) { fFilled = true; }
    grectf(char* n, float lx, float ly) : grect(n, lx, ly) { fFilled = true; }
    grectf(float xa, float ya, float xb, float yb) : grect(xa, ya, xb, yb) { fFilled = true; }
    grectf(char* n, float xa, float ya, float xb, float yb) : grect(n,xa,ya,xb,yb) { fFilled = true; }
};


#endif

