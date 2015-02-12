/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

#ifndef GSCENE_H
#define GSCENE_H

// System
#include <stdlib.h>

// Forward declarations
class frustumXZ;
class gcamera;
class gstage;


//===========================================================================
// gscene
//===========================================================================
class gscene
{
public:
    gscene();
	~gscene();
        
    void SetDrawLights(bool dl);
    void FixCamera(bool);
    void SetCamera(gcamera* camera);
    void SetStage(gstage* s);
    void Clear();  // issues ->Clear() to pstage
    const char* getname();
    
	void Draw();
	void Draw(const frustumXZ& fxz);
	void Print();
    
    bool PerspectiveSet();
    void UsePerspective();

private:
    void MakeCamera();

    gcamera* fCamera;
    gstage* fStage;
    const char* fName;
    bool fDrawLights;
    bool fCameraFixed;
    bool fMadeCamera;  // true if allocated room for default camera
    
};

inline void gscene::SetDrawLights(bool dl) { fDrawLights = dl; }
inline void gscene::SetStage(gstage* s) { fStage = s; }
inline const char* gscene::getname() { return fName; }

#endif

