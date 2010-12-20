/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// graphics.h: declaration of all graphics classes

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <iostream>

#include <gl.h>

#include "proplib.h"

struct Color
{
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;

	void set( GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a );
	void set( GLfloat _r, GLfloat _g, GLfloat _b );

	Color &operator=( proplib::Property &prop );
};
typedef struct Color Color;

std::ostream &operator<<( std::ostream &out, const Color &color );
// Read RGB from stream, sets alpha channel to 1.0
std::istream &operator>>( std::istream &in, Color &color );

#endif

