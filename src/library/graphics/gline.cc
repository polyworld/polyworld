
// Self
#include "gline.h"

// System
#include <gl.h>
#include <fstream>
#include <ostream>

// Local
#include "utils/misc.h"

using namespace std;

//===========================================================================
// gline
//===========================================================================

gline::gline()
{
	init();
}


gline::gline(char* n) : gobject(n)
{
	init();
}


gline::gline(float* e)
{
	setend(e);
}


gline::gline(float* b, float* e)
{
	settranslation(b); setend(e);
}


gline::gline(float b0, float b1, float b2, float* e)
{
	settranslation(b0,b1,b2);
	setend(e);
}


gline::gline(float b0, float b1, float b2, float e0, float e1, float e2)
{
	settranslation(b0, b1, b2);
	setend(e0, e1, e2);
}


void gline::init()
{
	fEnd[0] = 0.0;
	fEnd[1] = 0.0;
	fEnd[2] = 0.0;
}


void gline::setend(float* e)
{
	fEnd[0] = e[0];
	fEnd[1] = e[1];
	fEnd[2] = e[2];
}


void gline::setend(float e0, float e1, float e2)
{
	fEnd[0] = e0;
	fEnd[1] = e1;
	fEnd[2] = e2;
}


void gline::draw()
{
	glColor3fv(&fColor[0]);
	
	glBegin(GL_LINES);
		glVertex3fv(fPosition);
		glVertex3fv(fEnd);
	glEnd();
}


void gline::print()
{
    gobject::print();
    cout << "  fEnd-pt (x,y,z) = (" << fEnd[0] comma fEnd[1] comma fEnd[2] pnl;
    cout.flush();
}

