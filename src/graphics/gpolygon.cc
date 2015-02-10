
// Self
#include "gpolygon.h"

// System
#include <fstream>

// qt
#include <qapplication.h>

// Local
#include "misc.h"

using namespace std;

//===========================================================================
// gpoly
//===========================================================================

gpoly::gpoly()
{
	fNumPoints = 0;
	fVertices = 0;
	init();
}


gpoly::gpoly(long np,float* v)
{
	fNumPoints = np;
	fVertices = v;
	init();
}


gpoly::gpoly(char* n, long np,float* v) : gobject(n)
{
	fNumPoints = np;
	fVertices = v;
	init();
}


gpoly::gpoly(const opoly& p)
{
	fNumPoints = p.fNumPoints;
	fVertices = p.fVertices;
	init();
}


gpoly::gpoly(char* n, const opoly& p) : gobject(n)
{
	fNumPoints = p.fNumPoints;
	fVertices = p.fVertices;
	init();
}


gpoly::gpoly(const opoly* pp)
{
	fNumPoints = (*pp).fNumPoints;
	fVertices = (*pp).fVertices;
	init();
}


gpoly::gpoly(char* n, const opoly* pp) : gobject(n)
{
	fNumPoints = (*pp).fNumPoints;
	fVertices = (*pp).fVertices;
	init();
}


void gpoly::setradius(float r)
{
	fRadiusFixed = true;
	gobject::setradius(r);
	srPrint( "    gpoly::%s(r): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}


void gpoly::setradius()
{
    if( !fRadiusFixed )  //  only set radius anew if not set manually
        fRadius = sqrt( fLength[0]*fLength[0] + fLength[1]*fLength[1] + fLength[2]*fLength[2] ) * fRadiusScale * fScale * 0.5;
	srPrint( "gpoly::%s(): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}


void gpoly::setradiusscale(float s)
{
	fRadiusFixed = false;
	fRadiusScale = s;
	setradius();
}


void gpoly::setscale(float s)
{
	fScale = s;
	setradius();
}


void gpoly::draw()
{
	glColor3fv(&fColor[0]);
	
    glPushMatrix();
      position();
      glScalef(fScale, fScale, fScale);
      
		glBegin(GL_POLYGON);
			for (int j = 0; j < fNumPoints; j++)
				glVertex3fv(&fVertices[j*3]);
		glEnd();
      
    glPopMatrix();
}


void gpoly::print()
{
    gobject::print();
    for (int j = 0; j < fNumPoints; j++)
        cout << "  vertex# (x,y,z) = " << j << " ("
             << fVertices[j*3] comma fVertices[j*3+1] comma fVertices[j*3+2] pnl;
    cout.flush();
}


void gpoly::init()
{
    fRadiusFixed = false;
    fRadiusScale = 1.0;
}



#pragma mark -

gpolyobj::gpolyobj()
{
	init(0, NULL);
}


gpolyobj::gpolyobj(long np, opoly* p)
{
	init(np, p);
}


gpolyobj::gpolyobj(char* n, long np, opoly* p)
	:	gobject(n)
{
	init(np, p);
}


gpolyobj::~gpolyobj()
{
#if 1
	if (fPolygon != NULL)
	{
		for (long i = 0; i < fNumPolygons; i++)
		{
			if (fPolygon[i].fVertices != NULL)
				delete [] fPolygon[i].fVertices;
		}
		delete [] fPolygon;
	}
#endif
}


// WARNING:  this routine assumes the object to be cloned has the precise
// same topology as the current object, unless the current object has yet
// to be defined (in which case appropriate memory will be allocated).
void gpolyobj::clonegeom(const gpolyobj& inPolyObj)
{
    if (fPolygon == NULL)
    {
        fPolygon = new opoly[inPolyObj.fNumPolygons];
        Q_CHECK_PTR(fPolygon);

        for (long i = 0; i < inPolyObj.fNumPolygons; i++)
        {
			fPolygon[i].fVertices = new float[inPolyObj.fPolygon[i].fNumPoints * 3];
			Q_CHECK_PTR(fPolygon[i].fVertices);
        }
    }
    else
    {
    	printf("cloning with allocated mem\n");
    }
    
    fNumPolygons = inPolyObj.fNumPolygons;
    
    for (long i = 0; i < fNumPolygons; i++)
    {
        fPolygon[i].fNumPoints = inPolyObj.fPolygon[i].fNumPoints;
        for (long j = 0; j < fPolygon[i].fNumPoints * 3; j++)
            fPolygon[i].fVertices[j] = inPolyObj.fPolygon[i].fVertices[j];
    }
}


void gpolyobj::drawcolpolyrange(long i1, long i2, float* color)
{
	glColor3fv(color);
	glPolygonMode(GL_FRONT, GL_FILL);      

	for (long i = i1; i <= i2; i++)
	{
		glBegin(GL_POLYGON);
			for (long j = 0; j < fPolygon[i].fNumPoints; j++)
				glVertex3fv(&fPolygon[i].fVertices[j * 3]);

			// send the first point again to close it
			glVertex3fv(&fPolygon[i].fVertices[0]);
		glEnd();		
	}
}


void gpolyobj::draw()
{
    glPushMatrix();
      position();
      glScalef(fScale, fScale, fScale);
      drawcolpolyrange(0, fNumPolygons - 1, fColor);
    glPopMatrix();
}


void gpolyobj::print()
{
    gobject::print();
    for (int i = 0; i < fNumPolygons; i++)
    {
        for (int j = 0; j < fPolygon[i].fNumPoints; j++)
        {
            cout << "  fPolygon# vertex#  (x,y,z) = "
                 << i << "  " << j << " ("
                 << fPolygon[i].fVertices[j*3  ] comma
                    fPolygon[i].fVertices[j*3+1] comma
                    fPolygon[i].fVertices[j*3+2] pnl;
        }
    }
    cout.flush();
}


void gpolyobj::setradius()
{
	if( !fRadiusFixed )  //  only set radius anew if not set manually
		fRadius = sqrt( fLength[0]*fLength[0] + fLength[1]*fLength[1] + fLength[2]*fLength[2] ) * fRadiusScale * fScale * 0.5;
	srPrint( "gpolyobj::%s(): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}


void gpolyobj::init(long np, opoly* p)
{
	fNumPolygons = np;
	fPolygon = p;
	fRadiusFixed = false;
	fRadiusScale = 1.0;
	
	if (np && p != NULL)
		setlen();
}



// determine fLength[] (bounding box) and do a setradius()
void gpolyobj::setlen() 
{
    float xmin = fPolygon[0].fVertices[0];
    float xmax = fPolygon[0].fVertices[0];
    float ymin = fPolygon[0].fVertices[1];
    float ymax = fPolygon[0].fVertices[1];
    float zmin = fPolygon[0].fVertices[2];
    float zmax = fPolygon[0].fVertices[2];
    for (int i = 0; i < fNumPolygons; i++)
    {
        for (int j = 0; j < fPolygon[i].fNumPoints; j++)
        {
            xmin = xmin < fPolygon[i].fVertices[j*3  ] ? xmin : fPolygon[i].fVertices[j*3  ];
            xmax = xmax > fPolygon[i].fVertices[j*3  ] ? xmax : fPolygon[i].fVertices[j*3  ];
            ymin = ymin < fPolygon[i].fVertices[j*3+1] ? ymin : fPolygon[i].fVertices[j*3+1];
            ymax = ymax > fPolygon[i].fVertices[j*3+1] ? ymax : fPolygon[i].fVertices[j*3+1];
            zmin = zmin < fPolygon[i].fVertices[j*3+2] ? zmin : fPolygon[i].fVertices[j*3+2];
            zmax = zmax > fPolygon[i].fVertices[j*3+2] ? zmax : fPolygon[i].fVertices[j*3+2];
        }
    }
    fLength[0] = xmax - xmin;
    fLength[1] = ymax - ymin;
    fLength[2] = zmax - zmin;
    setradius();
}


void gpolyobj::setradius(float r)
{
	fRadiusFixed = true;
	gobject::setradius(r);
	srPrint( "    gpolyobj::%s(r): r=%g%s\n", __FUNCTION__, fRadius, fRadiusFixed ? "(fixed)" : "" );
}


void gpolyobj::setradiusscale(float s)
{
	fRadiusFixed = false;
	fRadiusScale = s;
	setradius();
}


void gpolyobj::setscale(float s)
{
	fScale = s;
	setradius();
}


#pragma mark -

istream& operator>>(istream& s, opoly& op)
{
    s >> op.fNumPoints;
    
    if (op.fNumPoints <= 0)
    {
        error(1,"invalid number of points (", op.fNumPoints, ") in polyobj");
	}      
    else
    {
        op.fVertices = new float[op.fNumPoints * 3];
        if (op.fVertices != NULL)
        {
            for (int i = 0; i < op.fNumPoints * 3; i++)
            {
				if (!(s >> op.fVertices[i]))  // catch error or eof
                {
                	if (s.eof())
                	{
						if (i != op.fNumPoints - 1)
							error(1, "premature end-of-file for polyobj");

                	}
                	else if (s.fail())
                	{
						error(1, "_fail reading polyobj file;",
							  " probably a formatting error");

                	}
                	else if (s.bad())
                	{
						error(1, "_bad error reading polyobj file;",
							  " characters probably lost");

                	}                	
                    break;  // for loop
                }
            }
        }
        else
        {
            error(1, "unable to allocate memory for polyobj's polys");
		}
    }
    return s;
}


void operator>>(const char* filename, gpolyobj& gpo)
{
	filebuf fb;
	    
    if (fb.open(filename, ios_base::in) == 0)
    {
		error(1,"unable to open polyobj file \"",filename,"\"");
	}       
    else
    {
        bool noerror = true;
        istream in(&fb);
        char version[16];
        in >> version;
        
        if (!strcmp(version, "pw1"))
        {
            in >> gpo.fNumPolygons;
            if (gpo.fNumPolygons <= 0)
            {
                error(1,"invalid number of polys (",gpo.fNumPolygons,
                        ") in \"",filename,"\"");
			}                        
            else
            {
                gpo.fPolygon = new opoly[gpo.fNumPolygons];
                if (gpo.fPolygon != NULL)
                {
                    for (long i = 0; i < gpo.fNumPolygons; i++)
                    {
                        if (!(in >> gpo.fPolygon[i]))  // catch error or eof
                        {
							if (in.eof())
		                	{
								if (i != gpo.fNumPolygons - 1)
									error(1, "premature end-of-file for polyobj");
		
		                	}
		                	else if (in.fail())
		                	{
								error(1, "_fail reading polyobj file;",
									  " probably a formatting error");
		
		                	}
		                	else if (in.bad())
		                	{
								error(1, "_bad error reading polyobj file;",
									  " characters probably lost");
							}
														
							noerror = false;
							break;  // for loop						
						}
						
                    }
                    
                    if (noerror)
                        gpo.setlen();  // determine lengths (bounding box)
                }
                else
                {
                    error(1,"unable to allocate memory for polyobj \"", filename,"\"");				
				}
			}
		}
        else
        {
            error(1,"object \"",filename, "\" is of unknown type (",version,")");
		}
		           
        fb.close();
    }
}


