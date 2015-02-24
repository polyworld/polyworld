 /********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/


// Self
#include "gcamera.h"

// System
#include <gl.h>
#include <glu.h>
#include <iostream>
#include <stdlib.h>

// Local
#include "graphics.h"
#include "utils/misc.h"

using namespace std;


// Internal defaults
const float kDefaultFOV = 90.0;
const float kDefaultAspect = 0.0;
const float kDefaultNear = 0.00001;
const float kDefaultFar = 10000.0;


// Orientation, like with gobjects, is handled by yaw, pitch, and roll
// (rotate-y, rotate-x, rotate-z) applied in that order.
// Also like gobjects, gcameras are assumed to begin life facing down the
// negative z-axis.

// One of the most common useages in PolyWorld will be cameras attached
// to "agent" objects.  Accordingly, the camera need only be declared,
// cam.AttachTo(agent) invoked once, and cam.Use() should be invoked by
// the scene to which the camera is attached.  cam.SetFOV() and possibly
// cam.SetNear() and cam.SetFar() may be used if the defaults are not adequate.
// Cam.SetTranslation(x,y,z) and cam.SetRotation(yaw,pitch,roll) can be used
// to set the camera's position and rotation with respect to the
// object to which it is attached (or to global origin if it is not
// attached to an object).

// The other methods, SetPerspective(), SetLookAt(), etc. are provided for
// closer compatability with the Iris gl functions, but will probably
// not be used in PolyWorld.


//===========================================================================
// gcamera
//===========================================================================

//---------------------------------------------------------------------------
// gcamera::gcamera
//---------------------------------------------------------------------------
gcamera::gcamera()
	:	fFOV(kDefaultFOV),
		fAspect(kDefaultAspect),
		fNear(kDefaultNear), // should probably be 0
		fFar(kDefaultFar),
		fUsingLookAt(false),
		fFollowObject(NULL),
		fPerspectiveFixed(false),
		fPerspectiveInUse(false),
		glFogOn(false)				// this will be turned on for cameras attached to agents at the SetGraphics() function
{
	fPosition[0] = 0.0;
    fPosition[1] = 0.0;
    fPosition[2] = 0.0;
    
    fFixationPoint[0] = 0.0;
    fFixationPoint[1] = 0.0;
    fFixationPoint[2] = 0.0;
    
    fAngle[0] = 0.0;
    fAngle[1] = 0.0;
    fAngle[2] = 0.0;
}


//---------------------------------------------------------------------------
// gcamera::~gcamera
//---------------------------------------------------------------------------
gcamera::~gcamera()
{
}    


//---------------------------------------------------------------------------
// gcamera::SetAspect
//---------------------------------------------------------------------------
void gcamera::SetAspect(float width, float height)
{

	fAspect = float(width) / float(height);
}


// The following methods, along with SetAspect(float a) above, are provided for
// closer compatability with the Iris gl functions, but may not be used
// in PolyWorld.

//---------------------------------------------------------------------------
// gcamera::SetPerspective
//---------------------------------------------------------------------------
void gcamera::SetPerspective(float fov, float a, float n, float f)
{

	fFOV = fov;
	fAspect = a;
	fNear = n;
	fFar = f;
}


//---------------------------------------------------------------------------
// gcamera::UsePerspective
//---------------------------------------------------------------------------
void gcamera::UsePerspective()
{

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fFOV, fAspect, fNear, fFar);

// GL Fog Debugging Info.
/*
	if( glIsEnabled(GL_FOG) )
	{
		cout << "GL FOG is enabled!" << endl;
		GLint*   glFogColor   = new GLint;
		GLint*   glFogIndex   = new GLint;
		GLfloat* glFogDensity = new GLfloat;
		GLint*   glFogStart   = new GLint;
		GLint*   glFogEnd     = new GLint;
		GLint*   glFogMode    = new GLint;

		glGetIntegerv(GL_FOG_COLOR,glFogColor);
		glGetIntegerv(GL_FOG_INDEX,glFogIndex);
		glGetFloatv(GL_FOG_DENSITY,glFogDensity);
		glGetIntegerv(GL_FOG_START,glFogStart);
		glGetIntegerv(GL_FOG_END,glFogEnd);
		glGetIntegerv(GL_FOG_MODE,glFogMode);
								
		cout << "Fog Color: "   << *glFogColor  << " // Index: " << *glFogIndex
			 << " // Density: " << *glFogDensity << " // Start: " << *glFogStart
			 << " // End: "     << *glFogEnd     << " // Mode: "  << *glFogMode << endl;
		
		delete glFogColor;
		delete glFogIndex;
		delete glFogDensity;
		delete glFogStart;
		delete glFogEnd;
		delete glFogMode;
	}
	else
	{
		cout << "------------ GL Fog is NOT enabled ---------- " << endl;
	}
*/	
	
	fPerspectiveInUse = true;
}


//---------------------------------------------------------------------------
// gcamera::UpdatePerspective
//---------------------------------------------------------------------------
void gcamera::UpdatePerspective()
{
	if (!fPerspectiveFixed)
		UsePerspective();
}


//---------------------------------------------------------------------------
// gcamera::Perspective
//---------------------------------------------------------------------------
void gcamera::Perspective(float fov, float a, float n, float f)
{
	SetPerspective(fov, a, n, f);
	UsePerspective();
}


//---------------------------------------------------------------------------
// gcamera::FixPerspective
//---------------------------------------------------------------------------      
void gcamera::FixPerspective(bool fixed, float width, float height)
{
	SetAspect(width, height);
	
	if (fixed)
		UsePerspective();
		
	fPerspectiveFixed = fixed;
}


#if 0
//---------------------------------------------------------------------------
// gcamera::SetLookAt
//---------------------------------------------------------------------------      
void gcamera::SetLookAt(float vx, float vy, float vz, float px, float py, float pz, float t)
{
	fPosition[0] = vx;
	fPosition[1] = vy;
	fPosition[2] = vz;
	
	fFixationPoint[0] = px;
	fFixationPoint[1] = py;
	fFixationPoint[2] = pz;
	
	fAngle[2] = t;  // 'z' (should this be negated?)
	
	fUsingLookAt = true;
}
#endif


//---------------------------------------------------------------------------
// gcamera::SetFixationPoint
//---------------------------------------------------------------------------      
void gcamera::SetFixationPoint(float x, float y, float z)
{
	fFixationPoint[0] = x;
	fFixationPoint[1] = y;
	fFixationPoint[2] = z;
			
	fUsingLookAt = true;
}


//---------------------------------------------------------------------------
// gcamera::UseLookAt
//---------------------------------------------------------------------------      
void gcamera::UseLookAt()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(fPosition[0],
			  fPosition[1],
			  fPosition[2],
			  fFixationPoint[0],
			  fFixationPoint[1],
			  fFixationPoint[2],
			  fAngle[0],
			  fAngle[1],
			  fAngle[2]);
}


#if 0
//---------------------------------------------------------------------------
// gcamera::LookAt
//---------------------------------------------------------------------------      
void gcamera::LookAt(float vx, float vy, float vz, float px, float py, float pz, float t)
{
	SetLookAt(vx, vy, vz, px, py, pz, t);
	UseLookAt();
}
#endif


//---------------------------------------------------------------------------
// gcamera::Use
//---------------------------------------------------------------------------      
void gcamera::Use()
{
	UpdatePerspective();  // will do nothing if (fPerspectiveFixed) is true

	
    if (fUsingLookAt)
    {
        UseLookAt();
    }
    else
    {
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glRotatef(-fAngle[2], 0.0, 0.0, 1.0); // roll  (z)
		glRotatef(-fAngle[1], 1.0, 0.0, 0.0); // pitch (x)
		glRotatef(-fAngle[0], 0.0, 1.0, 0.0); // yaw   (y)
				
		glTranslatef(-fPosition[0], -fPosition[1], -fPosition[2]);
        
        if (fFollowObject != NULL)
        	fFollowObject->inverseposition();		
    }
}


//---------------------------------------------------------------------------
// gcamera::print
//---------------------------------------------------------------------------      
void gcamera::print()
{
	cout << "For the camera named \"" << fName << "\"...\n";
    cout << "  fov = " << fFOV nl;
    cout << "  fAspect = " << fAspect nl;
    cout << "  fNear = " << fNear nl;
    cout << "  fFar = " << fFar nl;
    cout << "  position (x,y,z) = (" << fPosition[0] comma fPosition[1] comma fPosition[2] pnl;

    if (fUsingLookAt)
        cout << "  using LookAt, not heading, (x,y,z) = ("
             << fFixationPoint[0] comma fFixationPoint[1] comma fFixationPoint[2] pnl;
    else
        cout << "  using heading, not LookAt, (yaw,pitch,roll) = ("
             << fAngle[0] comma fAngle[1] comma fAngle[2] pnl;
             
    if (fFollowObject != NULL)
    {
        long address = long(fFollowObject);
        const char* objname = fFollowObject->GetName();
        cout << "  attached to object at " << address
             << " named: \"" << objname qnl;
    }
    else
    {
        cout << "  not attached to any object" nl;
	}
}
//---------------------------------------------------------------------------
// gcamera::SetFog
//---------------------------------------------------------------------------    
void gcamera::SetFog( bool fog, char function, float density, int end )
{
	if( fog )			
	{
		glEnable(GL_FOG);				// turn on Fog to give the agents depth perception

		if( function == 'L' )		// is it Linear?
		{

//			cout << "Function is Linear. end: " << end << endl;
			glFogi(GL_FOG_MODE, GL_LINEAR);
			glFogf(GL_FOG_START, fNear );							// fNear is zero by default
			glFogf(GL_FOG_END, end );		// the "end" parameter for linear fog.  

		}
		else if( function == 'E' )			// is it Exponential?
		{
//			cout << "Function is Exponential. Density: " << density << endl;
			glFogi(GL_FOG_MODE,GL_EXP);
			glFogf(GL_FOG_DENSITY, density );
		}
		else if( function != 'O' )			// is it not Off ?
		{
			cerr << "Do not know glFogFunction beginning with '" << function << "'" << endl;
		}
	}
	
	else
	{
//		cout << "Disabling GL FOG." << endl;
		glDisable(GL_FOG);		// Turn off the fog if for some reason we ever wanted agents to turn it off.	
	}
}
