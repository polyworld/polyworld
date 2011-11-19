#ifndef GPOINT_H
#define GPOINT_H

// Local
#include "gobject.h"


//===========================================================================
// gpoint
//===========================================================================

class gpoint : public gobject
{
public:
    gpoint();    
    gpoint(float* p);
    gpoint(float px, float py = 0.0, float pz = 0.0);
    gpoint(char* n);
    gpoint(char* n, float* p);
    gpoint(char* n, float px, float py, float pz);
        
    virtual void draw();
};

#endif
