/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// gobject.h: declarations of graphic object classes

#ifndef GOBJECT_H
#define GOBJECT_H

// System
#include <gl.h>
#include <iostream>
#include <math.h>

// Local
#include "error.h"
#include "graphics.h"


// Orientation, like with gcameras, is handled by yaw, pitch, and roll
// (rotate-y, rotate-x, rotate-z) applied in that order.
// Also like gcameras, gobjects are assumed to begin life facing down
// the positive x-axis.


//===========================================================================
// gobject
//===========================================================================
class gobject // graphical object
{
public:
    virtual void print();
    virtual void draw();
    
    void settranslation(float* p);
    void settranslation(float p0, float p1 = 0.0, float p2 = 0.0);
    void addtranslation(float p0, float p1 = 0.0, float p2 = 0.0);
    
    void setx(float x);
    void sety(float y);
    void setz(float z);
    void addx(float x);
    void addy(float y);
    void addz(float z);
    void setpos(int k, float p);
    
    void SetRotation(float yaw, float pitch, float roll);
    void setyaw(float yaw);
	void RotateAxis(float delta, float x, float y, float z);
    
    void setpitch(float pitch);
    void setroll(float roll);
    void addyaw(float yaw);
    void addpitch(float pitch);
    void addroll(float roll);
        
    float getyaw();
    float getpitch();
    float getroll();
    float yaw();
    float pitch();
    float roll();
    void setscale(float s);
    void setradius(float r);
    
    void setcol3(float* c);
    void setcol4(float* c);
    void setcolor(float r, float g, float b);
    void setcolor(float r, float g, float b, float t);
	void setcolor(const Color& c);
		
    void settransparency(float t);
    void SetRed(float r);
    void SetGreen(float g);
    void SetBlue(float b);
    float GetRed();
    float GetGreen();
    float GetBlue();
    
    void SetName(char* pc);
    char* GetName();
    
    float* getposptr();    
    void GetPosition(float* p);
    void GetPosition(float& px, float& py, float& pz);    
    float GetPosition(int i);
    
    float x();
    float y();
    float z();
    float radius();
    
    void translate();
    void rotate();
    void position();
    void inversetranslate();
    void inverserotate();
    void inverseposition();
    
private:
    bool fRotated;

protected:
    gobject();
    gobject(char* n);	
    virtual ~gobject();

    void init();
	void dump(std::ostream& out);
	void load(std::istream& in);

    float fPosition[3];
    float fAngle[3];
    float fScale;
    float fColor[4];  // default color (r,g,b,t) if no material is bound
    char* fName;
    float fRadius;  // for sphere of influence
};

inline void gobject::setx(float x) { fPosition[0] = x; }
inline void gobject::sety(float y) { fPosition[1] = y; }
inline void gobject::setz(float z) { fPosition[2] = z; }
inline void gobject::addx(float x) { fPosition[0] += x; }
inline void gobject::addy(float y) { fPosition[1] += y; }
inline void gobject::addz(float z) { fPosition[2] += z; }
inline void gobject::setpos(int k, float p) { fPosition[k] = p; }
inline float gobject::getyaw() { return fAngle[0]; }
inline float gobject::getpitch() { return fAngle[1]; }
inline float gobject::getroll() { return fAngle[2]; }
inline float gobject::yaw() { return fAngle[0]; }
inline float gobject::pitch() { return fAngle[1]; }
inline float gobject::roll() { return fAngle[2]; }
inline void gobject::setscale(float s) { fScale = s; }
inline void gobject::setradius(float r) { fRadius = r; srPrint( "gobject::%s(r): r=%g\n", __FUNCTION__, fRadius ); }
inline void gobject::settransparency(float t) { fColor[3] = t; }
inline void gobject::SetRed(float r) { fColor[0] = r; }
inline void gobject::SetGreen(float g) { fColor[1] = g; }
inline void gobject::SetBlue(float b) { fColor[2] = b; }
inline float gobject::GetRed() { return fColor[0]; }
inline float gobject::GetGreen() { return fColor[1]; }
inline float gobject::GetBlue() { return fColor[2]; }
inline char* gobject::GetName() { return fName; }
inline float* gobject::getposptr() { return &fPosition[0]; }
inline float gobject::GetPosition(int i) { return fPosition[i]; }
inline float gobject::x() { return fPosition[0]; }
inline float gobject::y() { return fPosition[1]; }
inline float gobject::z() { return fPosition[2]; }
inline float gobject::radius() { return fRadius; }

#endif


