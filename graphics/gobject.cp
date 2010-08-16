// Self
#include "gobject.h"

// System
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdlib.h>

// Local
#include "globals.h"
#include "gmisc.h"
#include "misc.h"

using namespace std;

#pragma mark -

gobject::gobject()
{
	init();
}


gobject::gobject(char* n)
{
	init();
	SetName(n);
}


gobject::~gobject()
{
	if (fName != NULL)
		delete fName;
}


void gobject::init()
{
	fName = NULL;
	fRotated = false;
	fRadius = 0.0;
	fPosition[0] = fPosition[1] = fPosition[2] = 0.0;
	fScale = 1.0;
	fAngle[0] = fAngle[1] = fAngle[2] = 0.0;
	fColor[0] = rand() / 32767.0;
	fColor[1] = rand() / 32767.0;
	fColor[2] = rand() / 32767.0;
	fColor[3] = 0.;
	listLink = NULL;
	fCarriedBy = NULL;
	fTypeNumber = 0;
	fCarryOffset[0] = 0.0;
	fCarryOffset[1] = 0.0;
	fCarryOffset[2] = 0.0;
}
 
    
void gobject::dump(ostream& out)
{
    if (fName)
        out << fName nl;
    else
        out << "NULL" nl;
    out << fPosition[0] sp fPosition[1] sp fPosition[2] nl;
    out << fAngle[0] sp fAngle[1] sp fAngle[2] nl;
    out << fColor[0] sp fColor[1] sp fColor[2] sp fColor[3] nl;
    out << fScale nl;
    out << fRadius nl;
    out << fRotated nl;
}


void gobject::load(istream& in)
{
    if (fName)
        delete fName;
    fName = new char[256];
    in >> fName;
    in >> fPosition[0] >> fPosition[1] >> fPosition[2];
    in >> fAngle[0] >> fAngle[1] >> fAngle[2];
    in >> fColor[0] >> fColor[1] >> fColor[2] >> fColor[3];
    in >> fScale;
    in >> fRadius;
    in >> fRotated;
}


void gobject::print()
{
   	cout << "For object named = \"" << fName << "\"...\n";
    cout << "  position = " << fPosition[0] << ", "
                            << fPosition[1] << ", "
                            << fPosition[2] nl;
    cout << "  fScale = " << fScale nl;
    cout << "  default color (r,g,b) = ("
         << fColor[0] comma fColor[1] comma fColor[2] pnl;
    cout << "  radius = " << fRadius nl;
    cout.flush();
}


void gobject::draw()
{
}


void gobject::SetName(const char* pc)
{
    fName = new char[strlen(pc)+1];
    if (fName == NULL)
        error(1,"Insufficient memory to store object fName");
    else
        strcpy(fName, pc);
}


void gobject::settranslation(float p0, float p1, float p2)
{
	fPosition[0] = p0;
	fPosition[1] = p1;
	fPosition[2] = p2;
}


void gobject::addtranslation(float p0, float p1, float p2)
{
	fPosition[0] += p0;
	fPosition[1] += p1;
	fPosition[2] += p2;
}


void gobject::settranslation(float* p)
{
	fPosition[0] = p[0];
	fPosition[1] = p[1];
	fPosition[2] = p[2];
}


void gobject::SetRotation(float yaw, float pitch, float roll)
{
	fAngle[0] = yaw;
	fAngle[1] = pitch;
	fAngle[2] = roll;
	fRotated = true;
}


void gobject::setyaw(float yaw)
{
	fAngle[0] = yaw;
	fRotated = true;
}


void gobject::RotateAxis(float delta, float x, float y, float z)
{
	fAngle[0] += delta * x;
	fAngle[1] += delta * y;
	fAngle[2] += delta * z;
	fRotated = true;
}

void gobject::setpitch(float pitch)
{
	fAngle[1] = pitch;
	fRotated = true;
}


void gobject::setroll(float roll)
{
	fAngle[2] = roll;
	fRotated = true;
}


void gobject::addyaw(float yaw)
{
	fAngle[0] += yaw;
	fRotated = true;
}


void gobject::addpitch(float pitch)
{
	fAngle[1] += pitch;
	fRotated = true;
}


void gobject::addroll(float roll)
{
	fAngle[2] += roll;
	fRotated = true;
}


void gobject::setcol3(float* c)
{
	fColor[0] = c[0];
	fColor[1] = c[1];
	fColor[2] = c[2];
}


void gobject::setcol4(float* c)
{
	fColor[0] = c[0];
	fColor[1] = c[1];
	fColor[2] = c[2];
	fColor[3] = c[3];
}

void gobject::setcolor(float r, float g, float b)
{
	fColor[0] = r;
	fColor[1] = g;
	fColor[2] = b;
}


void gobject::setcolor(float r, float g, float b, float t)
{
	fColor[0] = r;
	fColor[1] = g;
	fColor[2] = b;
	fColor[3] = t;
}


void gobject::setcolor(const Color& c)
{
	fColor[0] = c.r;
	fColor[1] = c.g;
	fColor[2] = c.b;
	fColor[3] = c.a;
}


void gobject::GetPosition(float* p)
{
	p[0] = fPosition[0];
	p[1] = fPosition[1];
	p[2] = fPosition[2];
}


void gobject::GetPosition(float& px, float& py, float& pz)
{
	px = fPosition[0];
	py = fPosition[1];
	pz = fPosition[2];
}


// you'd better know what you're doing if you invoke these...
// (they are intended for use between glPushMatrix()/glPopMatrix() pairs)
void gobject::translate()
{
	glTranslatef(fPosition[0], fPosition[1], fPosition[2]);
}


void gobject::rotate()
{
	if (fRotated)
	{
		glRotatef(fAngle[0], 0.0, 1.0, 0.0);	// y
		glRotatef(fAngle[1], 1.0, 0.0, 0.0); 	// x
		glRotatef(fAngle[2], 0.0, 0.0, 1.0);	// z
	}
}


void gobject::position()
{
	translate();
	rotate();
}


void gobject::inversetranslate()
{
	glTranslatef(-fPosition[0], -fPosition[1], -fPosition[2]);
}


void gobject::inverserotate()
{
	if (fRotated)
	{
		glRotatef(-fAngle[2], 0.0, 0.0, 1.0);	// z
		glRotatef(-fAngle[1], 1.0, 0.0, 0.0); 	// x
		glRotatef(-fAngle[0], 0.0, 1.0, 0.0);	// y
	}		
}


void gobject::inverseposition()
{
	inverserotate();
	inversetranslate();
}

bool gobject::IsCarrying( int type )
{
    itfor( gObjectList, fCarries, it )
    {
        gobject* o = *it;
        
		if( (o->getType() & type) != 0 )
			return true;
    }

	return false;
}

void gobject::PickedUp( gobject* carrier, float dy )
{
	debugcheck( "%s # %lu picked up by %s # %lu", OBJECTTYPE( this ), getTypeNumber(), OBJECTTYPE( carrier ), carrier->getTypeNumber() );
	
	fCarriedBy = carrier;
	fCarryOffset[0] = carrier->x() - x();
	fCarryOffset[1] = dy;
	fCarryOffset[2] = carrier->z() - z();
	fPosition[0] = carrier->x();
	fPosition[1] += dy;
	fPosition[2] = carrier->z();
}


void gobject::Dropped( void )
{
	debugcheck( "%s # %lu dropped by %s # %lu", OBJECTTYPE( this ), getTypeNumber(), OBJECTTYPE( fCarriedBy ), fCarriedBy->getTypeNumber() );
	
	fCarriedBy = NULL;
	fPosition[0] -= fCarryOffset[0];
	fPosition[1] -= fCarryOffset[1];
	fPosition[2] -= fCarryOffset[2];
	
	if( fPosition[0] < 0.0 )
		fPosition[0] = 0.0;
	else if( fPosition[0] > globals::worldsize )
		fPosition[0] = globals::worldsize;

	if( fPosition[2] > 0.0 )
		fPosition[2] = 0.0;
	else if( fPosition[2] < -globals::worldsize )
		fPosition[2] = -globals::worldsize;
	
}

