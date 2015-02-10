/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// System
#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

// qt
#include <qapplication.h>

// Local
#include "error.h"
#include "glight.h"
#include "gmisc.h"
#include "graphics.h"
#include "misc.h"
#include "objectlist.h"

// Self
#include "gstage.h"


using namespace std;


#define MAXLIGHTS 10 // TODO

//===========================================================================
// gstage
//===========================================================================

//---------------------------------------------------------------------------
// gstage::gstage
//---------------------------------------------------------------------------
gstage::gstage()
	:	fSetList(NULL),
		fPropList(NULL),
		fLightList(NULL),
		fLightModel(NULL),
		fDrawLights(false),
		fDisplayList(0)
{
}


//---------------------------------------------------------------------------
// gstage::~gstage
//---------------------------------------------------------------------------
gstage::~gstage()
{
}


//---------------------------------------------------------------------------
// gstage::SetCast
//---------------------------------------------------------------------------
void gstage::SetCast(TCastList* pc)
{
	fCastList = pc;
}


//---------------------------------------------------------------------------
// gstage::SetLights
//---------------------------------------------------------------------------
void gstage::SetLights(TGraphicsLightList* pl)
{
	if (pl->size() >= MAXLIGHTS)
	{
	     char msg[256];
	     char num[16];
	     
	     strcpy(msg, "Can only use MAXLIGHTS of the ");
	     sprintf(num, "%ld", (pl->size()) + 1);
	     strcat(msg, num);
	     strcat(msg," lights in this list");
	     error(0, msg);
	}
	    
    fLightList = pl;
}


//---------------------------------------------------------------------------
// gstage::SetCurrentCamera
//---------------------------------------------------------------------------
void gstage::SetCurrentCamera(gcamera* pcam)
{
	Q_CHECK_PTR(fCastList);
	fCastList->SetCurrentCamera(pcam);
}


//---------------------------------------------------------------------------
// gstage::Clear
//---------------------------------------------------------------------------
void gstage::Clear()
{
	// TODO ownership?
	if (fLightList != NULL)
		fLightList->clear();
	
	if (fSetList != NULL)	
		fSetList->clear();
		
	if (fPropList != NULL)	
		fPropList->clear();

	if (fCastList != NULL)
		fCastList->clear();
}


//---------------------------------------------------------------------------
// gstage::Compile
//---------------------------------------------------------------------------
void gstage::Compile()
{
	if( fDisplayList )
	{
		Decompile();
	}

	GLuint displayList = glGenLists( 1 );
	assert( displayList );

	glNewList( displayList, GL_COMPILE );
	{
		Draw();
	}
	glEndList();

	fDisplayList = displayList;
}


//---------------------------------------------------------------------------
// gstage::Decompile
//---------------------------------------------------------------------------
void gstage::Decompile()
{
	glDeleteLists( fDisplayList, 1 );
	fDisplayList = 0;
}


//---------------------------------------------------------------------------
// gstage::Draw
//---------------------------------------------------------------------------
void gstage::Draw()
{
	if( fDisplayList )
	{
		glCallList( fDisplayList );
	}
	else
	{
		if (fLightModel != NULL)
			fLightModel->Use();
    	
		if (fDrawLights && fLightList != NULL)
		{
			fLightList->Draw();  // does position() and Draw() for each
			fLightList->Use(); 	// does position() and bind() for each
		}

		if (fSetList != NULL)	        
			fSetList->Draw();

		if (fPropList != NULL)			
			fPropList->Draw();
		
		if (fCastList != NULL)
			fCastList->Draw();
	}
}


//---------------------------------------------------------------------------
// gstage::Draw
//---------------------------------------------------------------------------
void gstage::Draw(const frustumXZ& fxz)
{
    if (fLightModel != NULL)
    	fLightModel->Use();
    
	if (fDrawLights && fLightList != NULL)
	{
		fLightList->Draw(fxz); 	// does position() and Draw() for each
        fLightList->Use();       // does position() and bind() for each
    }
    
	if (fSetList != NULL)
		fSetList->Draw(); 	// Kludge to let ground plane always be drawn
							// However, some group immune to the frustum
							// is a good idea.
                               
	if (fPropList != NULL)
		fPropList->Draw(fxz);

	if (fCastList != NULL)
		fCastList->Draw(fxz);
}


//---------------------------------------------------------------------------
// gstage::Print
//---------------------------------------------------------------------------
void gstage::Print()
{
//  cout << "For the stage named \"" << name << "\"...\n";
//	cout << "* The cast at " << fCastList << ":" nl;
	if (fCastList != NULL)
		fCastList->Print();

//	cout << "* The set at " << fSetList << ":" nl;
	if (fSetList != NULL)
		fSetList->Print();
	      
//	cout << "* The props at " << fPropList << ":" nl;
	if (fPropList != NULL)
		fPropList->Print();

//	cout << "* The lights at " << fLightList << ":" nl;
	if (fLightList != NULL)
		fLightList->Print();

    if (fLightModel != NULL)
    {
//        cout << "* The lightmodel at " << fLightModel << ":" nl;
        fLightModel->print();
    }
    else
    {
        cout << "* There is no lighting model" nl;
	}       
}


//---------------------------------------------------------------------------
// gstage::AddObject
//---------------------------------------------------------------------------
void gstage::AddObject(gobject* po)
{
    Q_CHECK_PTR(po);
	fCastList->Add(po);
}


//---------------------------------------------------------------------------
// gstage::AddLight
//---------------------------------------------------------------------------
void gstage::AddLight(glight* pl)
{
	Q_CHECK_PTR(fLightList);
	
	if (fLightList->size() >= MAXLIGHTS)
	{
		char msg[256];
		char num[16];
		strcpy(msg,"Can only use MAXLIGHTS of the ");
		sprintf(num,"%ld", (fLightList->size()) + 1);
		strcat(msg, num);
		strcat(msg," lights in this list");
		error(0,msg);
	}
	fLightList->Add(pl);
}


//---------------------------------------------------------------------------
// gstage::RemoveLight
//---------------------------------------------------------------------------
void gstage::RemoveLight(glight* pl)
{
	Q_CHECK_PTR(fLightList);

#if 1
	fLightList->Remove(pl);
#else
	TGraphicsLightList::iterator iter = find(fLightList->begin(), fLightList->end(), pl);
	if (iter != fLightList->end())
		fLightList->erase(iter);
#endif
}


//---------------------------------------------------------------------------
// gstage::RemoveObject
//---------------------------------------------------------------------------
void gstage::RemoveObject(gobject* po)
{
	Q_CHECK_PTR(fCastList);

#if 1
	fCastList->Remove(po);
#else
	TCastList::iterator iter = find(fCastList->begin(), fCastList->end(), po);
	if (iter != fCastList->end())
		fCastList->erase(iter);
#endif
}

