
// Self
#include "gsquare.h"

// System
#include <gl.h>

// Local
#include "gmisc.h"
#include "graphics.h"
#include "utils/misc.h"


using namespace std;

gsquare::gsquare()
{
	init();
}


gsquare::gsquare(char* n)
	:	gobject(n)
{
	init();
}


gsquare::gsquare(float lx, float ly)
{
	setsquare(lx,ly);
}


gsquare::gsquare(char* n, float lx, float ly)
	:	gobject(n)
{
	setsquare(lx,ly);
}


gsquare::gsquare(float xa, float ya, float xb, float yb)
{
	setsquare(xa, ya, xb, yb);
}


gsquare::gsquare(char* n, float xa, float ya, float xb, float yb)
	:	gobject(n)
{
	setsquare(xa, ya, xb, yb);
}


void gsquare::draw()
{
	glColor3fv(&fColor[0]);
	
	glPushMatrix();
		position();
		glScalef(fScale, fScale, fScale);
		if (fFilled)
			glRectf(-0.5 * fLengthX, -0.5 * fLengthY, 0.5 * fLengthX, 0.5 * fLengthY); // [TODO] fill it
		else
			glRectf(-0.5 * fLengthX, -0.5 * fLengthY, 0.5 * fLengthX, 0.5 * fLengthY);
    glPopMatrix();
}


void gsquare::print()
{
    gobject::print();
    cout << "  fLengthX = " << fLengthX nl;
    cout << "  fLengthY = " << fLengthY nl;
    if (fFilled)
        cout << "  it is fFilled" nl;
    else
        cout << "  it is not fFilled" nl;
    cout.flush();
}


void gsquare::init()
{
	fLengthX = fLengthY = 1.0;
	fRadiusFixed = false;
	fRadiusScale = 1.0;
}


void gsquare::setsquare(float lx, float ly)
{
	fLengthX = lx;
	fLengthY = ly;
	setradius();
}


void gsquare::setsquare(float lx, float ly, float x, float y)
{
	fLengthX = lx;
	fLengthY = ly;
	fPosition[0] = x;
	fPosition[1] = y;
	setradius();
}


void gsquare::setrect(float xa, float ya, float xb, float yb)
{
	float xp;
	float yp;
	xp = (xa + xb) * 0.5;
	yp = (ya + yb) * 0.5;
	fLengthX = fabs(xb - xa);
	fLengthY = fabs(yb - ya);
	settranslation(xp, yp);
	setradius();
}


void gsquare::setradius(float r)
{
	fRadiusFixed = true;
	gobject::setradius(r);
	srPrint( "    gsquare::%s(r): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}


void gsquare::setradiusscale(float s)
{
	fRadiusFixed = false;
	fRadiusScale = s;
	setradius();
}


void gsquare::setscale(float s)
{
	fScale = s;
	setradius();
}


void gsquare::setradius()
{
	if (!fRadiusFixed)  //  only set radius anew if not set manually
		fRadius = sqrt(fLengthX * fLengthX + fLengthY * fLengthY) * fRadiusScale * fScale * 0.5;
}    

void gbox::setradius()
{
    if (!fRadiusFixed)  //  only set radius anew if not set manually
        fRadius = sqrt(fLength[0] * fLength[0] + fLength[1] * fLength[1] + fLength[2] * fLength[2])
            * fRadiusScale * fScale * 0.5;
}


void gbox::draw()
{
	glColor3fv(&fColor[0]);
	
	glPushMatrix();
		position();		
		glScalef(fScale * fLength[0], fScale * fLength[1], fScale * fLength[2]);      
		//if (fFilled)		
			drawunitcube();
		//else
		//	frameunitcube();
					
    glPopMatrix();
}


void gbox::print()
{
    gobject::print();
    cout << "  Length X = " << fLength[0] nl;
    cout << "  Length Y = " << fLength[1] nl;
    cout << "  Length X = " << fLength[2] nl;
    if (fFilled)
        cout << "  it is fFilled" nl;
    else
        cout << "  it is not fFilled" nl;
    cout.flush();
}
