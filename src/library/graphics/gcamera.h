/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// gcamera.h: declarations of graphic camera classes

#ifndef GCAMERA_H
#define GCAMERA_H

// Local
#include "gsquare.h"


//===========================================================================
// gcamera
//===========================================================================
class gcamera : public gboxf  
{
public:    
	gcamera();
    ~gcamera();
    
	void SetFOV(float fov);
	float GetFOV();
	void SetNear(float n);
	void SetFar(float f);        
	void SetAspect(float width, float height);
	void SetAspect(float a);
	void SetFog( bool fog, char function, float density, int end );

	void Use();
    virtual void print();
    
	void AttachTo(gobject* gobj);
        
	void SetPerspective(float fov, float a, float n, float f);
	void SetTwist(float t);
	void UsePerspective();
    void UpdatePerspective();
	void Perspective(float fov, float a, float n, float f);
	void FixPerspective(bool pf, float width, float height);
	bool PerspectiveSet();
//	void SetLookAt(float vx, float vy, float vz, float px, float py, float pz, float t);
	void SetFixationPoint(float x, float y, float z);        
	void UseLookAt();    
	void LookAt(float vx, float vy, float vz, float px, float py, float pz, float t);

	
private:
	float fFOV; // short
	float fAspect;
	float fNear;
	float fFar;
	float fFixationPoint[3]; // fixation point; lookat point
	bool fUsingLookAt;
	gobject* fFollowObject;
	bool fPerspectiveFixed;
	bool fPerspectiveInUse;
	bool glFogOn;			// this will be turned on for agents at the Attach() function

	char sFogFunction;
	float fExpFogDensity;
	int   iLinearFogEnd;
	
};


//===========================================================================
// inlines
//===========================================================================
inline void gcamera::AttachTo(gobject* gobj) { fFollowObject = gobj; }
inline void gcamera::SetFOV(float fov) { fFOV = fov; }    
inline float gcamera::GetFOV() { return fFOV; }
inline void gcamera::SetNear(float n) { fNear = n; }
inline void gcamera::SetFar(float f) { fFar = f; }
inline void gcamera::SetAspect(float a) { fAspect = a; }
//inline void gcamera::SetTwist(float t) { fAngle[2] = t; }
inline bool gcamera::PerspectiveSet() { return fPerspectiveInUse; }

#endif
