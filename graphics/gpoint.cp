
// System
#include <gl.h>

// Self
#include "gpoint.h"

gpoint::gpoint()
{

}

gpoint::gpoint::gpoint(float* p)
{
	settranslation(p);
}

gpoint::gpoint(float px, float py, float pz)
{
	settranslation(px, py, pz);
}
 
gpoint::gpoint(char* n) : gobject(n)
{

}

gpoint::gpoint(char* n, float* p) : gobject(n)
{
	settranslation(p);
}

gpoint::gpoint(char* n, float px, float py, float pz) : gobject(n)
{
	settranslation(px, py, pz);
}



void gpoint::draw()
{
	glColor3fv(&fColor[0]);
	
	glBegin(GL_POINTS);
		glVertex3fv(fPosition);
	glEnd();
}

