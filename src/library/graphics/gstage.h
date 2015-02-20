/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// gstage.h: declaration of gstage classes

#ifndef GSTAGE_H
#define GSTAGE_H

#include <gl.h>

// Local
#include "gmisc.h"

// Forward declarations
class frustumXZ;
class gcamera;
class glight;
class glightmodel;
class gobject;

class gstage
{
public:
    gstage();
    ~gstage();
    
    void SetSet(TSetList* ps) { fSetList = ps; }
    void SetProps(TPropList* pp) { fPropList = pp; }    
    void SetCast(TCastList* cast);    
    void SetLights(TGraphicsLightList* lights);
    void SetDrawLights(bool dl) { fDrawLights = dl; }
    void SetLightModel(glightmodel* plm) { fLightModel = plm; }    
    void SetCurrentCamera(gcamera* pcam);
    
    void Clear();  // issues ->Clear() for all associated lists
	void Compile();
	void Decompile();
	void Draw();
	void Draw(const frustumXZ& fxz);
	void Print();
    
	// The following are added mostly for some quick & dirty testing.
	// Use the cast, set, props, and lights list plus the camera pointer
	// for real applications.
    void AddObject(gobject* po);        
    void RemoveObject(gobject* po);

    void AddLight(glight* pl);
    void RemoveLight(glight* pl);
	        	
private:
	TSetList* fSetList;
    TPropList* fPropList;
    TGraphicsLightList* fLightList;
    glightmodel* fLightModel;
    bool fDrawLights;
    TCastList* fCastList; 
	GLuint fDisplayList;
};

#endif

