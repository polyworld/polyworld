/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// gscene.C: implementation of gscene classes

// Self
#include "gscene.h"

// System
#include <iostream>
#include <gl.h>

// qt
#include <qapplication.h>

// Local
#include "gcamera.h"
#include "graphics.h"
#include "gstage.h"
#include "misc.h"


using namespace std;


//===========================================================================
// gscene
//===========================================================================


//---------------------------------------------------------------------------
// gscene::gscene
//---------------------------------------------------------------------------
gscene::gscene()
	:	fCamera(NULL),
        fName("Stage"),
		fDrawLights(false),
		fCameraFixed(false),
		fMadeCamera(false)	
{
}



//---------------------------------------------------------------------------
// gscene::~gscene
//---------------------------------------------------------------------------
gscene::~gscene()
{
	if (fMadeCamera)
		delete fCamera;
}



//---------------------------------------------------------------------------
// gscene::MakeCamera
//---------------------------------------------------------------------------
void gscene::MakeCamera()
{
    fCamera = new gcamera();  // just take defaults
    Q_CHECK_PTR(fCamera);
    fMadeCamera = true;
}



//---------------------------------------------------------------------------
// gscene::FixCamera
//---------------------------------------------------------------------------
void gscene::FixCamera(bool cf)
{
    if (cf)
    {
		if (fCamera != NULL)
			fCamera->Use();
		else
			error(1,"can't fix a scene's camera when one has not been set");
    }
    fCameraFixed = cf;
}


//---------------------------------------------------------------------------
// gscene::SetCamera
//
// We only delete the camera pointer if we have ownership of it
//---------------------------------------------------------------------------
void gscene::SetCamera(gcamera* pc)
{
    if (fMadeCamera)
	{
		delete fCamera;
		fMadeCamera = false;
	}
	
    fCamera = pc;
}


//---------------------------------------------------------------------------
// gscene::Clear
//---------------------------------------------------------------------------
void gscene::Clear()
{
    if (fStage != NULL)
    	fStage->Clear();
}


//---------------------------------------------------------------------------
// gscene::Draw
//---------------------------------------------------------------------------
void gscene::Draw()
{
	if (fCamera == NULL)
    	MakeCamera();
    	
	glPushMatrix();
		if (!fCameraFixed)
			fCamera->Use();
      
		if (fStage != NULL)
		{
			fStage->SetCurrentCamera(fCamera);
			fStage->SetDrawLights(fDrawLights);
			fStage->Draw();
		}      
	glPopMatrix();
}



//---------------------------------------------------------------------------
// gscene::Draw
//---------------------------------------------------------------------------
void gscene::Draw(const frustumXZ& fxz)
{
    if (fCamera == NULL)
    	MakeCamera();
    	
    glPushMatrix();
		if (!fCameraFixed)
			fCamera->Use();
			
		if (fStage != NULL)
      	{
			fStage->SetCurrentCamera(fCamera);
			fStage->SetDrawLights(fDrawLights);
			fStage->Draw(fxz);
		}
    glPopMatrix();
}



//---------------------------------------------------------------------------
// gscene::Print
//---------------------------------------------------------------------------
void gscene::Print()
{
    cout << "For the scene named \"" << fName << "\"...\n";
    if (fCamera != NULL)
    {
        cout << "**The camera at " << fCamera << ":" nl;
        fCamera->print();
        cout << "**The camera is ";
        if (!fCameraFixed)
        	cout << "not ";
        cout << "fixed" nl;
    }
    else
        cout << "**There is no camera" nl;
    if (fStage != NULL)
    {
        cout << "**The stage at " << fStage << ":" nl;
        fStage->Print();
    }
    else
        cout << "**There is no stage" nl;
    cout << "**The lights in this scene will ";
    if (!fDrawLights) cout << "not ";
    cout << "be drawn" nl;
}


//---------------------------------------------------------------------------
// gscene::PerspectiveSet
//---------------------------------------------------------------------------
bool gscene::PerspectiveSet()
{
	if (fCamera != NULL)
		return fCamera->PerspectiveSet();
	else
		return false;
}


//---------------------------------------------------------------------------
// gscene::UsePerspective
//---------------------------------------------------------------------------
void gscene::UsePerspective()
{
	if (fCamera != NULL)
		fCamera->UsePerspective();
}


