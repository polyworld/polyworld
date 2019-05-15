// Self
#include "ChartMonitorView.h"

// System
#include <iostream>

#include <glu.h>
#include <stdio.h>

// Local
#include "monitor/Monitor.h"
#include "utils/error.h"
#include "utils/misc.h"

using namespace std;

//===========================================================================
// ChartMonitorView
//===========================================================================

#define FIXED_WIDTH 325
#define FIXED_HEIGHT 150

//---------------------------------------------------------------------------
// ChartMonitorView::ChartMonitorView
//---------------------------------------------------------------------------
ChartMonitorView::ChartMonitorView( ChartMonitor *monitor )
	: MonitorView( monitor, FIXED_WIDTH, FIXED_HEIGHT, true )
{
	init( (short)monitor->getCurveDefs().size(),
		  FIXED_WIDTH,
		  FIXED_HEIGHT );

	citfor( ChartMonitor::CurveDefs, monitor->getCurveDefs(), it_curve )
	{
		const ChartMonitor::CurveDef &curve = *it_curve;

		setRange( curve.id, curve.range[0], curve.range[1] );
		if( curve.color[0] >= 0.0 )
			setColor( curve.id, curve.color[0], curve.color[1], curve.color[2] );
	}

    monitor->curveUpdated += [=](short curve, float data) {this->addPoint(curve, data);};
}

//---------------------------------------------------------------------------
// ChartMonitorView::~ChartMonitorView
//---------------------------------------------------------------------------
ChartMonitorView::~ChartMonitorView()
{
	// Clean up
	delete y;
	delete numPoints;
	delete lowV;
	delete highV;
	delete dydv;
	delete color;
}


//---------------------------------------------------------------------------
// ChartMonitorView::init
//---------------------------------------------------------------------------
void ChartMonitorView::init( short ncurves, int width, int height )
{
	lowX = 35;
	highX = width - 10;
	lowY = 5;
	highY = height - 5;
	maxPoints = highX - lowX + 1;
	decimation = 0;
	numCurves = ncurves;
	y = NULL;

	numPoints = new long[numCurves];
	lowV = new float[numCurves];
	highV = new float[numCurves];
	dydv = new float[numCurves];
	color = new Color[numCurves];

	for (short ic = 0; ic < numCurves; ic++)
	{
		numPoints[ic] = 0;
		lowV[ic] = 0.;
		highV[ic] = 1.;
		dydv[ic] = float(highY - lowY) / (highV[ic] - lowV[ic]);

		if (numCurves > 1)
		{
			color[ic].r = float(ic) / float(numCurves-1);
			color[ic].g =      1.  -  0.5 * float(ic) / float(numCurves-1);
			color[ic].b = fabs(1.  -  2.0 * float(ic) / float(numCurves-1));
		}
		else
		{
			color[ic].r = 0.;
			color[ic].g = 1.;
			color[ic].b = 1.;
		}
	}
}


//---------------------------------------------------------------------------
// ChartMonitorView::paintGL
//---------------------------------------------------------------------------
void ChartMonitorView::paintGL()
{
	glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, width(), 0.0, height());
		glMatrixMode(GL_MODELVIEW);

		draw();
	glPopMatrix();
}


//---------------------------------------------------------------------------
// ChartMonitorView::draw()
//---------------------------------------------------------------------------
void ChartMonitorView::draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	drawAxes();
	plotPoints();
}


//---------------------------------------------------------------------------
// ChartMonitorView::initializeGL
//---------------------------------------------------------------------------
void ChartMonitorView::initializeGL()
{
	qglClearColor( Qt::black );
    glShadeModel( GL_SMOOTH );
}


//---------------------------------------------------------------------------
// ChartMonitorView::resizeGL
//---------------------------------------------------------------------------
void ChartMonitorView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width, 0.0, height);
	glMatrixMode(GL_MODELVIEW);
}

//---------------------------------------------------------------------------
// ChartMonitorView::plotPoints
//---------------------------------------------------------------------------
void ChartMonitorView::plotPoints()
{
	for (short ic = 0; ic < numCurves; ic++)
	{
		glColor3fv((GLfloat *)&color[ic]);
		glLineWidth(1.0);

		glBegin(GL_POINTS);
		for (long i = 0; i < numPoints[ic]; i++)
		{
			if ((y[(ic * maxPoints) + i] >= lowY) && (y[(ic * maxPoints) + i] <= highY))
				glVertex2f(i + lowX, y[(ic * maxPoints) + i]);
		}
		glEnd();
	}
}


//---------------------------------------------------------------------------
// ChartMonitorView::plotPoint
//---------------------------------------------------------------------------
void ChartMonitorView::plotPoint(short index, long x, long y)
{
    if ((y >= lowY) && (y <= highY) )
    {
		glColor3fv((GLfloat *)&color[index]);
		glBegin(GL_POINTS);
        	glVertex2f(x, y);
		glEnd();
    }
}


//---------------------------------------------------------------------------
// ChartMonitorView::drawAxes
//---------------------------------------------------------------------------
void ChartMonitorView::drawAxes()
{
	char s[256];
    long y0;

	qglColor( Qt::white );

    if ((lowV[0]<=0.0) && (highV[0]>=0.0))
        y0 = (long)((highY-lowY) * (0.0 - lowV[0]) / (highV[0] - lowV[0]) + lowY);
    else
        y0 = lowY;

    // x-axis
	glBegin(GL_LINES);
    	glVertex2f(lowX - 2, y0);
    	glVertex2f(highX, y0);
	glEnd();

    // y-axis
	glBegin(GL_LINES);
    	glVertex2f(lowX, lowY);
    	glVertex2f(lowX, highY);
	glEnd();

    // label the y-axis (but not the x)
    glBegin(GL_LINES);
    	glVertex2f(lowX - 2, lowY);
    	glVertex2f(lowX + 2, lowY);
	glEnd();


	QFont font("Monospace", 8, QFont::Normal);
	font.setStyleHint(QFont::TypeWriter);
	QFontMetrics metrics(font);

    if (highV[0] > 1.0 && highV[0] == highV[0] && lowV[0] == lowV[0])
		sprintf(s, "%ld", long(lowV[0]));
    else
        sprintf(s,"%.1f", lowV[0]);
	qglColor( Qt::white );
	renderText(lowX - metrics.width(s) - 2, highY, s, font);

	glBegin(GL_LINES);
    	glVertex2f(lowX - 2, highY);
    	glVertex2f(lowX + 2, highY);
    glEnd();

    if (highV[0] > 1.0 && highV[0] == highV[0])
        sprintf(s, "%ld", long(highV[0]));
    else
        sprintf(s, "%.1f",highV[0]);
	qglColor( Qt::white );
	renderText(lowX - metrics.width(s) - 3, lowY + (metrics.height() / 2) + 1, s, font);

    if (decimation)
    {
		long x = (lowX + highX) / 2;
        long y = lowY - 2;
        while ((y + 4) <= highY)
        {
            glBegin(GL_LINES);
    			glVertex2f(x, y);
    			glVertex2f(x, y + 4);
    		glEnd();
            y += 8;
        }
    }
}


//---------------------------------------------------------------------------
// ChartMonitorView::setRange
//---------------------------------------------------------------------------
void ChartMonitorView::setRange(short ic, float valmin, float valmax)
{
    if (valmin < valmax)
    {
        lowV[ic] = valmin;
        highV[ic] = valmax;
    }
    else if (valmax < valmin)
    {
        lowV[ic] = valmax;
        highV[ic] = valmin;
    }
    else
    {
        error(1,"attempt to define null range for chart window");
        lowV[ic] = valmin;
        highV[ic] = valmin+1.;
    }

    dydv[ic] = (highY - lowY) / (highV[ic] - lowV[ic]);
}


//---------------------------------------------------------------------------
// ChartMonitorView::setColor
//---------------------------------------------------------------------------
void ChartMonitorView::setColor(short ic, const Color& col)
{
    color[ic].r = col.r;
    color[ic].g = col.g;
    color[ic].b = col.b;
}


//---------------------------------------------------------------------------
// ChartMonitorView::setColor
//---------------------------------------------------------------------------
void ChartMonitorView::setColor(short ic, float r, float g, float b)
{
    color[ic].r = r;
    color[ic].g = g;
    color[ic].b = b;
}


//---------------------------------------------------------------------------
// ChartMonitorView::addPoint
//---------------------------------------------------------------------------
void ChartMonitorView::addPoint( short ic, float val )
{
    long i;
    long j;
	bool repaint = false;

    if( y == NULL )  // first time must allocate space
    {
        y = new long[maxPoints * numCurves];
        Q_CHECK_PTR(y);
    }

    if (numPoints[ic] == maxPoints)
	{
        // cut the number (and resolution) of points stored in half
        for( long jc = 0; jc < numCurves; jc++ )
        {
            for( i = 1, j = 2; j < min(numPoints[jc], maxPoints); i++, j += 2 )
                y[(jc*maxPoints)+i] = y[(jc*maxPoints)+j];
            numPoints[jc] = i;
        }
        decimation++;
		repaint = true;
    }

	y[(long)((ic * maxPoints) + numPoints[ic])] = (long)((val - lowV[ic]) * dydv[ic]  +  lowY);

	if( isVisible() )
	{
		makeCurrent();
		if( repaint )
			draw();
		else
			plotPoint( ic, numPoints[ic] + lowX, y[(ic * maxPoints) + numPoints[ic]] );
		swapBuffers();
	}
    numPoints[ic]++;
}


//---------------------------------------------------------------------------
// ChartMonitorView::load
//---------------------------------------------------------------------------
void ChartMonitorView::load(std::istream& in)
{
    long newnumcurves;
    in >> newnumcurves;

    if (newnumcurves != numCurves)
        error(2,"while loading from dump file, numCurves redefined");

    in >> decimation;

    if (y == NULL)  // first time must allocate space
    {
        y = new long[maxPoints * numCurves];
        if (y == NULL)
        {
            error(2,"insufficient space for chartwindow points during load");
            return;
        }
    }

    for (long ic = 0; ic < numCurves; ic++)
    {
        in >> numPoints[ic];
        for (long i = 0; i < numPoints[ic]; i++)
        {
            in >> y[(ic*maxPoints)+i];
        }
    }
}


//---------------------------------------------------------------------------
// ChartMonitorView::dump
//---------------------------------------------------------------------------
void ChartMonitorView::dump(std::ostream& out)
{
    out << numCurves nl;
    out << decimation nl;
    for (long ic = 0; ic < numCurves; ic++)
    {
        out << numPoints[ic] nl;
        for (long i = 0; i < numPoints[ic]; i++)
        {
			out << y[(ic*maxPoints)+i] nl;
        }
    }
}

