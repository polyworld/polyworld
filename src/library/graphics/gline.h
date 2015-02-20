#ifndef GLINE_H
#define GLINE_H

// Local
#include "gobject.h"

//===========================================================================
// gline
//===========================================================================
class gline : public gobject
{
public:
    gline();
    gline(char* n);
    gline(float* e);
    gline(float* b, float* e);
    gline(float b0, float b1, float b2, float* e);
    gline(float b0, float b1, float b2, float e0, float e1, float e2);
    
	void init();

    void setend(float* e);
    void setend(float e0, float e1, float e2);
    
    virtual void draw();
    virtual void print();

private:
    float fEnd[3];  // beginning is considered to be at gobject::fPosition[]
};

#endif

