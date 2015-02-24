/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// gmisc.cp: implementation of miscellaneous graphics classes

#define DebugFrustum 0

// System
#include <gl.h>
#include <math.h>

// Local
#include "glight.h"
#include "gobject.h"
#include "graphics.h"
#include "sim/globals.h"
#include "utils/misc.h"

// Self
#include "gmisc.h"


static const float ucube[8][3] = { {-0.5, -0.5, -0.5},
			                       {-0.5, -0.5,  0.5},
			                       {-0.5,  0.5, -0.5},
			                       {-0.5,  0.5,  0.5},
			                       { 0.5, -0.5, -0.5},
			                       { 0.5, -0.5,  0.5},
			                       { 0.5,  0.5, -0.5},
			                       { 0.5,  0.5,  0.5} };



//===========================================================================
// CompareRadius (predicate)
//===========================================================================
struct CompareRadius
{
	bool operator() (gobject* item1, gobject* item2) const
		{
			return (item1->x() - item1->radius() < item2->x() - item2->radius());
		}
};


//-------------------------------------------------------------------------------------------
// TGraphicObjectList::Add
//-------------------------------------------------------------------------------------------
void TGraphicObjectList::Add(gobject* itemToAdd)
{
	bool inserted = false;

	TGraphicObjectList::iterator iter = begin();	
	for (; iter != end(); ++iter) 
	{
		gobject* listItem = *iter;
		
		if ((itemToAdd->x() - itemToAdd->radius()) < (listItem->x() - listItem->radius()))
        {
            insert(iter, itemToAdd);
            inserted = true;
            break;
        }
    }
    
    // Add to the end if insertion point not found
    if (!inserted)
    	push_back(itemToAdd);
}


//-------------------------------------------------------------------------------------------
// TGraphicObjectList::Sort
//-------------------------------------------------------------------------------------------
void TGraphicObjectList::Sort()
{
	// This technique assumes that the list is almost entirely sorted at the start
	// Hopefully, with some reasonable frame-to-frame coherency, this will be true!
	sort(CompareRadius());
}


//-------------------------------------------------------------------------------------------
// TGraphicObjectList::Draw
//-------------------------------------------------------------------------------------------
void TGraphicObjectList::Draw()
{
	TGraphicObjectList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		gobject* obj = *iter;
				
		if (obj != (gobject*)fCurrentCamera)
        	obj->draw();
	}
}


//-------------------------------------------------------------------------------------------
// TGraphicObjectList::Draw
//-------------------------------------------------------------------------------------------
void TGraphicObjectList::Draw(const frustumXZ& fxz)
{
//	cout << "Drawing with frustum(x0, z0, angmin, angmax) = "
//		 << fxz.x0 comma fxz.z0 comma fxz.angmin comma fxz.angmax pnlf;

	TGraphicObjectList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		gobject* obj = *iter;
				
		if (obj != (gobject*)fCurrentCamera)
		{
//          float* p = pobj->getposptr();
//          cout << "  object at (x,z) = (" << p[0] comma p[2] << ") ";
            if( fxz.Inside( obj->getposptr() ) )//object position inside frustum?
            {
//              cout << "IS     ";
                obj->draw();
            }
//          else
//              cout << "is NOT ";
//          cout << "inside" nlf;		
		}
	}
}


//-------------------------------------------------------------------------------------------
// TGraphicObjectList::Print
//-------------------------------------------------------------------------------------------
void TGraphicObjectList::Print()
{
	TGraphicObjectList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		gobject* obj = *iter;
		obj->print();		
	}
}


//-------------------------------------------------------------------------------------------
// TLightList::Draw
//-------------------------------------------------------------------------------------------
void TLightList::Draw()
{
	TLightList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		glight* light = *iter;
		light->Draw();		
	}
}


//-------------------------------------------------------------------------------------------
// TLightList::Draw
//-------------------------------------------------------------------------------------------
void TLightList::Draw(const frustumXZ& fxz)
{
	TLightList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		glight* light = *iter;
        if (fxz.Inside(light->getposptr())) // object position inside frustum?
			light->Draw();		
    }
}



//-------------------------------------------------------------------------------------------
// TLightList::Use
//-------------------------------------------------------------------------------------------
void TLightList::Use()
{
	int count = 0;
	
	TLightList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		if (++count > MAXLIGHTS)
		{
			error(1,"Attempted to use more than MAXLIGHTS lights");
			break;
		}
				 
		glight* light = *iter;
		light->Use(count - 1);  // subtract 1 due to 0-based light numbering
	}
	
	//for (int index = count; i < MAXLIGHTS; i++)
	//	lmbind(LIGHT0+i,0);  // make sure no old lights are left bound
	
}	


//-------------------------------------------------------------------------------------------
// TLightList::Print
//-------------------------------------------------------------------------------------------
void TLightList::Print()
{
	TLightList::const_iterator iter = begin();
	for (; iter != end(); ++iter)
	{
		glight* light = *iter;
		light->Print();		
	}
}



//-------------------------------------------------------------------------------------------
// drawunitcube
//-------------------------------------------------------------------------------------------
void drawunitcube()
{
    glPolygonMode(GL_FRONT, GL_FILL);
    
	glBegin(GL_POLYGON);
		glVertex3fv(ucube[0]);
		glVertex3fv(ucube[1]);
		glVertex3fv(ucube[3]);
		glVertex3fv(ucube[2]);
	glEnd();
	
	glBegin(GL_POLYGON);
		glVertex3fv(ucube[0]);
		glVertex3fv(ucube[4]);
		glVertex3fv(ucube[5]);
		glVertex3fv(ucube[1]);
	glEnd();
	
	glBegin(GL_POLYGON);
		glVertex3fv(ucube[4]);
		glVertex3fv(ucube[6]);
		glVertex3fv(ucube[7]);
		glVertex3fv(ucube[5]);
	glEnd();
	
	glBegin(GL_POLYGON);
		glVertex3fv(ucube[2]);
		glVertex3fv(ucube[3]);
		glVertex3fv(ucube[7]);
		glVertex3fv(ucube[6]);
	glEnd();
	
	glBegin(GL_POLYGON);
		glVertex3fv(ucube[5]);
		glVertex3fv(ucube[7]);
		glVertex3fv(ucube[3]);
		glVertex3fv(ucube[1]);
	glEnd();
	
	glBegin(GL_POLYGON);
		glVertex3fv(ucube[0]);
		glVertex3fv(ucube[2]);
		glVertex3fv(ucube[6]);
		glVertex3fv(ucube[4]);
	glEnd();
}


//-------------------------------------------------------------------------------------------
// frameunitcube
//-------------------------------------------------------------------------------------------
void frameunitcube()
{
	glBegin(GL_LINES);
		glVertex3fv(ucube[0]);
		glVertex3fv(ucube[1]);
		glVertex3fv(ucube[3]);
		glVertex3fv(ucube[2]);
		
		glVertex3fv(ucube[0]);
		glVertex3fv(ucube[4]);
		glVertex3fv(ucube[6]);
		glVertex3fv(ucube[2]);
		
		glVertex3fv(ucube[3]);
		glVertex3fv(ucube[7]);
		glVertex3fv(ucube[6]);
		glVertex3fv(ucube[4]);
		
		glVertex3fv(ucube[5]);
		glVertex3fv(ucube[1]);
		glVertex3fv(ucube[5]);
		glVertex3fv(ucube[7]);
	glEnd();
}


//-------------------------------------------------------------------------------------------
// frustumXZ
//-------------------------------------------------------------------------------------------
void frustumXZ::Set(float x, float z, float ang, float fov)
{
#if DebugFrustum
	cout << "frustum being set with (x,z,ang,fov) = ("
		 << x comma z comma ang comma fov pnlf;
#endif
    x0 = x;
    z0 = z;
    angmin = fmod((ang - 0.5*fov)*DEGTORAD,TWOPI);
    if (fabs(angmin) > PI) angmin -= (angmin > 0.0) ? TWOPI : (-TWOPI);
    angmax = fmod((ang + 0.5*fov)*DEGTORAD,TWOPI);
    if (fabs(angmax) > PI) angmax -= (angmin > 0.0) ? TWOPI : (-TWOPI);
#if DebugFrustum
	cout << "resulting frustum is (x0,z0,angmin,angmax) = ("
		 << x0 comma z0 comma angmin comma angmax pnlf;
#endif
}


void frustumXZ::Set(float x, float z, float ang, float fov, float rad)
{
#if DebugFrustum
	cout << "frustum being set with (x,z,ang,fov,rad) = ("
		 << x comma z comma ang comma fov comma rad pnlf;
#endif
    float x1 = x + rad * sin(ang*DEGTORAD) / sin(fov*0.5*DEGTORAD);
    float z1 = z + rad * cos(ang*DEGTORAD) / sin(fov*0.5*DEGTORAD);
    Set(x1, z1, ang, fov);
}


static long infrustum = 0;
static long outfrustum = 0;

int frustumXZ::Inside(float* p) const
{
    float ang = atan2(x0 - p[0], z0 - p[2]);
#if DebugFrustum
	cout << "ang" ses ang cms "angmin" ses angmin cms "angmax" ses angmax nlf;
#endif
    if (angmin < angmax)
    {
        if (ang < angmin)
        {
            outfrustum++;
            return 0;
        }
        else if (ang > angmax)
        {
            outfrustum++;
            return 0;
        }
        else
        {
            infrustum++;
            return 1;
        }
    }
    else
    {
        if (ang > angmin)
        {
            infrustum++;
            return 1;
        }
        else if (ang < angmax)
        {
            infrustum++;
            return 1;
        }
        else
        {
            outfrustum++;
            return 0;
        }
    }
}
