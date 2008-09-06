/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// Self
#include "glight.h"

// Local
#include "globals.h"
#include "gobject.h"
#include "graphics.h"
#include "indexlist.h"
#include "misc.h"

using namespace std;


// for constructing the properties array to pass to lmdef
float glprops[6];

// statics
indexlist* glight::pindices;
indexlist* glightmodel::pindices;


glight::glight()
{
#if 0
    if (pindices == NULL)
    	pindices = new indexlist(1, 65535);
    	
    index = short((*pindices).getindex());
    if (index < 1)
    {
        error(1, "Attempt to define more than 65535 light types");
	}
    else
    {
//        lmdef(DEFLIGHT, index, 0, (const float[])NULL);
        name = NULL;
        dims = 3;
        ambient[0] = ambient[1] = ambient[2] = 0.0;
        col[0] = col[1] = col[2] = 1.0; col[3] = 0.4; // translucent for draw
        pos[0] = pos[1] = pos[3] = 0.0;
        pos[2] = 1.0;
        pattobj = NULL;
#endif
}

glight::~glight()
{
	(*pindices).freeindex(index);
}


void glight::settranslation(float x, float y, float z, float w)
{
//    glprops[0] = POSITION;
    glprops[1] = x;
    glprops[2] = y;
    glprops[3] = z;
    glprops[4] = w;
//    glprops[5] = LMNULL;
//    lmdef(DEFLIGHT,index,6,glprops);
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    pos[3] = w;
}


void glight::setcol(float r, float g, float b)
{
//    glprops[0] = LCOLOR;
    glprops[1] = r;
    glprops[2] = g;
    glprops[3] = b;
//    glprops[4] = LMNULL;
//    lmdef(DEFLIGHT,index,5,glprops);
    col[0] = r;
    col[1] = g;
    col[2] = b;
}


void glight::setdims(int d)
{
    if ((d<1) || (d>3))
    {
        char errmsg[256],cdims[16];
        strcpy(errmsg,"dimensionality must 1, 2, or 3 (not ");
        sprintf(cdims,"%d",dims);
        strcat(errmsg,cdims);
        strcat(errmsg,")");
        error(1,errmsg);
    }
    else
        dims = d;
}


// 0 <= lightnum <= MAXLIGHTS
void glight::bind(short lightnum)
{
    if ((lightnum < 0) || (lightnum > MAXLIGHTS))
    {
        char errmsg[256],lnum[16];
        strcpy(errmsg, "Light number must be >= 0 and < MAXLIGHTS (not ");
        sprintf(lnum,"%d", lightnum);
        strcat(errmsg,lnum);
        strcat(errmsg,")");
        error(1,errmsg);
    }
    else
    {
//        lmbind(LIGHT0+lightnum,index);  // 110# == LIGHT#
    }
}


void glight::Draw()
{
//	c4f(col);
    glPushMatrix();
//    loadmatrix(identmat);
      position();
//      drawunitcube(); // make this a sphere some day
    glPopMatrix();
}


void glight::Use(short lightnum)
{
///    c4f(col);
	glPushMatrix();
//    loadmatrix(identmat);
      position();
      bind(lightnum);
    glPopMatrix();
}


void glight::Print()
{
   cout << "For light name = " << name nl;
   cout << "  light dimensionality = " << dims nl;
   switch (dims)
   {
   case 1:
      cout << "  light position = " << pos[0] nl;
      break;
   case 2:
      cout << "  light position = " << pos[0] cms
                                       pos[1] nl;
      break;
   case 3:
      cout << "  light position = " << pos[0] cms
                                       pos[1] cms
                                       pos[2] nl;
      break;
   }
}


void glight::position()
{
	if (pattobj != NULL)
		pattobj->position();
	translate();
}



glightmodel::glightmodel()
{
    if (pindices == NULL)
    	pindices = new indexlist(1, 65535);
    	
    index = short((*pindices).getindex());
    
    if (index < 1)
    {
        error(1,"Attempt to define more than 65535 lighting models");
	}
    else
    {
        ambient[0] = ambient[1] = ambient[2] = 0.2;
        localviewer = 0.0;
        attenuation[0] = 1.0;
        attenuation[1] = 0.0;
//        lmdef(DEFLMODEL, index, 0, (const float[])NULL);
    }
}


glightmodel::~glightmodel()
{
	(*pindices).freeindex(index);
}


void glightmodel::setambient(float r, float g, float b)
{
	//glprops[0] = AMBIENT;
    glprops[1] = r;
    glprops[2] = g;
    glprops[3] = b;
//    glprops[4] = LMNULL;
 //   lmdef(DEFLMODEL,index,5,glprops);
    ambient[0] = r;
    ambient[1] = g;
    ambient[2] = b;
}


void glightmodel::setlocalviewer(bool lv)
{
//	glprops[0] = LOCALVIEWER;
    
    if (lv)
    	glprops[1] = 1.0;
	else
		glprops[1] = 0.0;
		
//    glprops[2] = LMNULL;
//    lmdef(DEFLMODEL,index,3,glprops);
    localviewer = glprops[1];
}


void glightmodel::setattenuation(float fixed, float variable)
{
//    glprops[0] = ATTENUATION;
    glprops[1] = fixed;
    glprops[2] = variable;
//    glprops[3] = LMNULL;
//    lmdef(DEFLMODEL,index,4,glprops);
    attenuation[0] = fixed;
    attenuation[1] = variable;
}


void glightmodel::bind()
{
//	lmbind(LMODEL, index); TODO
}


void glightmodel::Use()
{
//	bind();
}


void glightmodel::print()
{
    cout << "For lighting model number " << index << "...\n";
    cout << "  ambient color (r,g,b) = " << ambient[0] cms
                                            ambient[1] cms
                                            ambient[2] nl;
    cout << "  attenuation (fixed,variable) = " << attenuation[0] cms
                                                   attenuation[1] nl;
    cout << "  localviewer = " << (localviewer == 1.0) nl;
}

