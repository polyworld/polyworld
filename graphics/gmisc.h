/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// gmisc.h: declaration of miscellaneous graphics classes

#ifndef GMISC_H
#define GMISC_H

// System
#include <stddef.h>

// Local
#include "objectlist.h"

// Forward declarations
class gcamera;
class glight;
class gobject;

void drawunitcube();
void frameunitcube();


//===========================================================================
// frustumXZ
//===========================================================================
class frustumXZ
{
public:
    frustumXZ() { }
    frustumXZ(float x, float z, float ang, float fov) { Set(x, z, ang, fov); }
    frustumXZ(float x, float z, float ang, float fov, float rad) { Set(x, z, ang, fov, rad); }
    ~frustumXZ() { }
    
    int Inside(float* p) const;
    
    void Set(float x, float z, float ang, float fov);
    void Set(float x, float z, float ang, float fov, float rad);

    float x0;
    float z0;
    float angmin;
    float angmax;
};


//===========================================================================
// TGraphicObjectList
//===========================================================================
class TGraphicObjectList : public ObjectList<gobject *>  
{
public:
    TGraphicObjectList() : fCurrentCamera(NULL) {}
	virtual ~TGraphicObjectList() { }

    void SetCurrentCamera(const gcamera* pcam) { fCurrentCamera = pcam; }
    
    virtual void Add(gobject* item);
    virtual void Sort();

    virtual void Draw();
    virtual void Draw(const frustumXZ& fxz);
    
    void Print();
                
protected:	
    const gcamera* fCurrentCamera;
};


//===========================================================================
// TLightList
//===========================================================================

// graphic lights list (adds draw to regular doubly linked list)
class TLightList : public ObjectList<glight *>
{
public:
    TLightList() { }
	virtual ~TLightList() { }
	
    virtual void Draw();
    virtual void Draw(const frustumXZ& fxz);
    
    void Use();
    void Print();

protected:

};

//===========================================================================
// TCastList
//===========================================================================
// graphics cast (assumed to be mostly dynamic objects)
class TCastList : public TGraphicObjectList
{
public:
    TCastList() { }
    virtual ~TCastList() { }
};


//===========================================================================
// TSetList
//===========================================================================
// graphics set (assumed to be mostly (all?) static objects)
class TSetList : public TGraphicObjectList
{
public:
    TSetList() { }
    virtual ~TSetList() { }
};


//===========================================================================
// TPropList
//===========================================================================
// graphics props (assumed to be a mix of static & dynamic objects)
class TPropList : public TGraphicObjectList
{
public:
    TPropList() { }
    virtual ~TPropList() { }
};


//===========================================================================
// TGraphicsLightList
//===========================================================================
// graphics lights (assumed to be a mix of static & dynamic objects)
class TGraphicsLightList : public TLightList
{
public:
    TGraphicsLightList() { }
	virtual ~TGraphicsLightList() { }
};

#endif


