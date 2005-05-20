//---------------------------------------------------------------------------
//	File:		ChartWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "ChartWindow.h"

// System
#include <iostream>

// qt
#include <qapplication.h>
#include <qgl.h>
#include <qsettings.h>

// Local
#include "error.h"
#include "globals.h"
#include "misc.h"

using namespace std;

//===========================================================================
// TChartWindow
//===========================================================================

int TChartWindow::kMaxWidth = 325;
int TChartWindow::kMaxHeight = 150;

//---------------------------------------------------------------------------
// TChartWindow::TChartWindow
//---------------------------------------------------------------------------
TChartWindow::TChartWindow(const char* name)
	:	QGLWidget(NULL, name, NULL, WStyle_Customize | WStyle_SysMenu | WStyle_Tool)
{
	Init(1);
}


//---------------------------------------------------------------------------
// TChartWindow::TChartWindow
//---------------------------------------------------------------------------
TChartWindow::TChartWindow(const char* name, short numCurves)
	:	QGLWidget(NULL, name, NULL, WStyle_Customize | WStyle_SysMenu | WStyle_Tool)
{	
	Init(numCurves);
}



//---------------------------------------------------------------------------
// TChartWindow::~TChartWindow
//---------------------------------------------------------------------------
TChartWindow::~TChartWindow()
{
	SaveDimensions();
	
	// Clean up
	delete y;
	delete fNumPoints;
	delete fLowV;
	delete fHighV;
	delete dydv;
	delete fColor;
}


//---------------------------------------------------------------------------
// TChartWindow::Init
//---------------------------------------------------------------------------
void TChartWindow::Init(short ncurves)
{
//	setDoubleBuffer( false );
//	printf( "doubleBuffer() = %s\n", BoolString( doubleBuffer() ) );
	fLowX = 0;
	fHighX = 100;
	fLowY = 0;
	fHighY = 100;
	fMaxPoints = fHighX - fLowX + 1;
	fDecimation = 0;
	fNumCurves = ncurves;
	y = NULL;
		
	fNumPoints = new long[fNumCurves];
	fLowV = new float[fNumCurves];
	fHighV = new float[fNumCurves];
	dydv = new float[fNumCurves];
	fColor = new Color[fNumCurves];
	
	for (short ic = 0; ic < fNumCurves; ic++)
	{
		fNumPoints[ic] = 0;
		fLowV[ic] = 0.;
		fHighV[ic] = 1.;
		dydv[ic] = float(fHighY - fLowY) / (fHighV[ic] - fLowV[ic]);
		
		if (fNumCurves > 1)
		{
			fColor[ic].r = float(ic) / float(fNumCurves-1);
			fColor[ic].g =      1.  -  0.5 * float(ic) / float(fNumCurves-1);
			fColor[ic].b = fabs(1.  -  2.0 * float(ic) / float(fNumCurves-1));
		}
		else
		{
			fColor[ic].r = 0.;
			fColor[ic].g = 1.;
			fColor[ic].b = 1.;
		}
	}
}

//---------------------------------------------------------------------------
// TChartWindow::paintGL
//---------------------------------------------------------------------------
void TChartWindow::paintGL()
{
	// Prepare the canvas for drawing
	glPushMatrix();
//		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, width(), 0.0, height());
		glMatrixMode(GL_MODELVIEW);
		
//		DrawAxes();
//	    PlotPoints();
		Draw();
	glPopMatrix();
}


//---------------------------------------------------------------------------
// TChartWindow::Draw()
//---------------------------------------------------------------------------
void TChartWindow::Draw()
{
		glClear(GL_COLOR_BUFFER_BIT);
		DrawAxes();
	    PlotPoints();	
}


//---------------------------------------------------------------------------
// TChartWindow::initializeGL
//---------------------------------------------------------------------------
void TChartWindow::initializeGL()
{
	qglClearColor(black);
    glShadeModel(GL_SMOOTH);
}


//---------------------------------------------------------------------------
// TChartWindow::resizeGL
//---------------------------------------------------------------------------
void TChartWindow::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width, 0.0, height);	
	glMatrixMode(GL_MODELVIEW);
}
   
    
//---------------------------------------------------------------------------
// TChartWindow::mousePressEvent
//---------------------------------------------------------------------------
void TChartWindow::mousePressEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TChartWindow::mouseMoveEvent
//---------------------------------------------------------------------------
void TChartWindow::mouseMoveEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TChartWindow::mouseReleaseEvent
//---------------------------------------------------------------------------
void TChartWindow::mouseReleaseEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TChartWindow::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TChartWindow::mouseDoubleClickEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TChartWindow::PlotPoints
//---------------------------------------------------------------------------
void TChartWindow::PlotPoints()
{
	for (short ic = 0; ic < fNumCurves; ic++)
	{
		glColor3fv((GLfloat *)&fColor[ic]);
		glLineWidth(1.0);
		
		glBegin(GL_POINTS);
		//glBegin(GL_LINE_STRIP);				
		for (long i = 0; i < fNumPoints[ic]; i++)
		{
			if ((y[(ic * fMaxPoints) + i] >= fLowY) && (y[(ic * fMaxPoints) + i] <= fHighY))
				glVertex2f(i + fLowX, y[(ic * fMaxPoints) + i]);
		}		
		glEnd();
	}
}


//---------------------------------------------------------------------------
// TChartWindow::PlotPoint
//---------------------------------------------------------------------------
void TChartWindow::PlotPoint(short index, long x, long y)
{
    if ((y >= fLowY) && (y <= fHighY) )
    {
		glColor3fv((GLfloat *)&fColor[index]);
		glBegin(GL_POINTS);
        	glVertex2f(x, y);
		glEnd();
    }
}


//---------------------------------------------------------------------------
// TChartWindow::DrawAxes
//---------------------------------------------------------------------------
void TChartWindow::DrawAxes()
{
	char s[256];
    long y0;

	qglColor(white);
	
    if ((fLowV[0]<=0.0) && (fHighV[0]>=0.0))
        y0 = (long)((fHighY-fLowY) * (0.0 - fLowV[0]) / (fHighV[0] - fLowV[0]) + fLowY);
    else
        y0 = fLowY;

    // x-axis
	glBegin(GL_LINES);
    	glVertex2f(fLowX - 2, y0);
    	glVertex2f(fHighX, y0);
	glEnd();    	
    
    // y-axis
	glBegin(GL_LINES);
    	glVertex2f(fLowX, fLowY);
    	glVertex2f(fLowX, fHighY);
	glEnd();
	
    // label the y-axis (but not the x)
    glBegin(GL_LINES);
    	glVertex2f(fLowX - 2, fLowY);
    	glVertex2f(fLowX + 2, fLowY);
	glEnd();


	QFont font("helvetica", 12, QFont::Normal);
	QFontMetrics metrics(font);
	    
    if (fHighV[0] > 1.0 && fHighV[0] == fHighV[0] && fLowV[0] == fLowV[0])
		sprintf(s, "%ld", long(fLowV[0]));
    else
        sprintf(s,"%.1f", fLowV[0]);
	qglColor(white); 
	renderText(fLowX - metrics.width(s) - 2, fHighY + (metrics.height() / 2) + 3, s, font);

	glBegin(GL_LINES);
    	glVertex2f(fLowX - 2, fHighY);
    	glVertex2f(fLowX + 2, fHighY);
    glEnd();
    
    if (fHighV[0] > 1.0 && fHighV[0] == fHighV[0])
        sprintf(s, "%ld", long(fHighV[0]));
    else
        sprintf(s, "%.1f",fHighV[0]);
	qglColor(white);        
	renderText(fLowX - metrics.width(s) - 2, fLowY + (metrics.height() / 2) + 3, s, font);

    if (fDecimation)
    {
		long x = (fLowX + fHighX) / 2;
        long y = fLowY - 2;
        while ((y + 4) <= fHighY)
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
// TChartWindow::SetRange
//---------------------------------------------------------------------------
void TChartWindow::SetRange(short ic, float valmin, float valmax)
{
    if (valmin < valmax)
    {
        fLowV[ic] = valmin;
        fHighV[ic] = valmax;
    }
    else if (valmax < valmin)
    {
        fLowV[ic] = valmax;
        fHighV[ic] = valmin;
    }
    else
    {
        error(1,"attempt to define null range for chart window");
        fLowV[ic] = valmin;
        fHighV[ic] = valmin+1.;
    }
    
    dydv[ic] = (fHighY - fLowY) / (fHighV[ic] - fLowV[ic]);
}


//---------------------------------------------------------------------------
// TChartWindow::SetColor
//---------------------------------------------------------------------------
void TChartWindow::SetColor(short ic, const Color& col)
{
    fColor[ic].r = col.r;
    fColor[ic].g = col.g;
    fColor[ic].b = col.b;
}


//---------------------------------------------------------------------------
// TChartWindow::SetColor
//---------------------------------------------------------------------------
void TChartWindow::SetColor(short ic, float r, float g, float b)
{
    fColor[ic].r = r;
    fColor[ic].g = g;
    fColor[ic].b = b;
}


//---------------------------------------------------------------------------
// TChartWindow::AddPoint
//---------------------------------------------------------------------------
void TChartWindow::AddPoint( short ic, float val )
{
    long i;
    long j;
    
    if( y == NULL )  // first time must allocate space
    {
        y = new long[fMaxPoints * fNumCurves];
        Q_CHECK_PTR(y);
    }
    
    if (fNumPoints[ic] == fMaxPoints)
	{
        // cut the number (and resolution) of points stored in half
        for( long jc = 0; jc < fNumCurves; jc++ )
        {
            for( i = 1, j = 2; j < min(fNumPoints[jc], fMaxPoints); i++, j += 2 )
                y[(jc*fMaxPoints)+i] = y[(jc*fMaxPoints)+j];
            fNumPoints[jc] = i;
        }
        fDecimation++;
		makeCurrent();
		paintGL();
    }
        
	y[(long)((ic * fMaxPoints) + fNumPoints[ic])] = (long)((val - fLowV[ic]) * dydv[ic]  +  fLowY);    
	
	if( isShown() )
	{
		makeCurrent();
		PlotPoint( ic, fNumPoints[ic] + fLowX, y[(ic * fMaxPoints) + fNumPoints[ic]] );
		swapBuffers();
	}
    fNumPoints[ic]++;
}


//---------------------------------------------------------------------------
// TChartWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TChartWindow::RestoreFromPrefs(long x, long y)
{
	// Set up some defaults
	int defWidth = kMaxWidth;
	int defHeight = kMaxHeight;
	int defX = x;
	int defY = y;	
	bool visible = true;
	
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			defWidth = settings.readNumEntry("/width", defWidth);
			defHeight = settings.readNumEntry("/height", defHeight);
			defX = settings.readNumEntry("/x", defX);
			defY = settings.readNumEntry("/y", defY);
			visible = settings.readBoolEntry("/visible", visible);

		settings.endGroup();
	settings.endGroup();
	
	// Pin values
	if (defWidth > kMaxWidth)
		defWidth = kMaxWidth;
		
	if (defHeight > kMaxHeight)
		defHeight = kMaxHeight;
		
	QDesktopWidget *d = QApplication::desktop();
	if (defX < d->x() || defX > d->width())
		defX = 0;

	if (defY < d->y() || defY > d->height())
		defY = 60;

	// Set window size and location based on prefs
	setFixedSize(defWidth, defHeight);
	QRect position = geometry();
	position.moveTopLeft(QPoint(defX, defY));
  	setGeometry(position);
	
	// Set up default settings
    fLowX = 25;
	fHighX = width() - 10; // fLowX + width() - 10;
    fLowY = 5;
    fHighY = height() - 10; // height() - 10;

	fMaxPoints = fHighX - fLowX + 1;
    
	for (short ic = 0; ic < fNumCurves; ic++)
		dydv[ic] = float(fHighY - fLowY) / (fHighV[ic] - fLowV[ic]);
	
	if (visible)
		show();
		
	// Save settings for future restore		
	SaveDimensions();		
}


//---------------------------------------------------------------------------
// TChartWindow::Load
//---------------------------------------------------------------------------
void TChartWindow::Load(std::istream& in)
{
    long newnumcurves;
    in >> newnumcurves;
    
    if (newnumcurves != fNumCurves)
        error(2,"while loading from dump file, fNumCurves redefined");
        
    in >> fDecimation;
    
    if (y == NULL)  // first time must allocate space
    {
        y = new long[fMaxPoints * fNumCurves];
        if (y == NULL)
        {
            error(2,"insufficient space for chartwindow points during load");
            return;
        }
    }
    
    for (long ic = 0; ic < fNumCurves; ic++)
    {
        in >> fNumPoints[ic];
        for (register long i = 0; i < fNumPoints[ic]; i++)
        {
            in >> y[(ic*fMaxPoints)+i];
        }
    }
}


//---------------------------------------------------------------------------
// TChartWindow::Dump
//---------------------------------------------------------------------------
void TChartWindow::Dump(std::ostream& out)
{
    out << fNumCurves nl;
    out << fDecimation nl;
    for (long ic = 0; ic < fNumCurves; ic++)
    {
        out << fNumPoints[ic] nl;
        for (long i = 0; i < fNumPoints[ic]; i++)
        {
			out << y[(ic*fMaxPoints)+i] nl;
        }
    }
}


//---------------------------------------------------------------------------
// TChartWindow::EnableAA
//---------------------------------------------------------------------------
void TChartWindow::EnableAA()
{
	// Set up antialiasing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//---------------------------------------------------------------------------
// TChartWindow::DisableAA
//---------------------------------------------------------------------------
void TChartWindow::DisableAA()
{
	// Set up antialiasing
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}


//---------------------------------------------------------------------------
// TChartWindow::SaveDimensions
//---------------------------------------------------------------------------
void TChartWindow::SaveDimensions()
{
	// Save size and location to prefs
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			settings.writeEntry("/width", geometry().width());
			settings.writeEntry("/height", geometry().height());			
			settings.writeEntry("/x", geometry().x());
			settings.writeEntry("/y", geometry().y());
			
		settings.endGroup();
	settings.endGroup();
}


//---------------------------------------------------------------------------
// TChartWindow::SaveVisibility
//---------------------------------------------------------------------------
void TChartWindow::SaveVisibility()
{
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			settings.writeEntry("/visible", isShown());

		settings.endGroup();
	settings.endGroup();
}


//---------------------------------------------------------------------------
// TChartWindow::customEvent
//---------------------------------------------------------------------------
void TChartWindow::customEvent(QCustomEvent* event)
{
	if (event->type() == kUpdateEventType)
		updateGL();
}


//===========================================================================
// TBinChartWindow
//===========================================================================

//---------------------------------------------------------------------------
// TBinChartWindow::TBinChartWindow
//---------------------------------------------------------------------------
TBinChartWindow::TBinChartWindow(const char* name)
	:	TChartWindow(name)
{
	Init();
}


//---------------------------------------------------------------------------
// TBinChartWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TBinChartWindow::RestoreFromPrefs(long x, long y)
{
	// Set up some defaults
	int defWidth = kMaxWidth;
	int defHeight = kMaxHeight;
	int defX = x;
	int defY = y;
	bool visible = true;
	int titleHeight = 16;
	
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			defWidth = settings.readNumEntry("/width", defWidth);
			defHeight = settings.readNumEntry("/height", defHeight);
			defX = settings.readNumEntry("/x", defX);
			defY = settings.readNumEntry("/y", defY);
			visible = settings.readBoolEntry("/visible", visible);
			
		settings.endGroup();
	settings.endGroup();
	
	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft(QPoint(defX, defY));
 	position.setSize(QSize(defWidth, defHeight));
  	setGeometry(position);
	setFixedSize(defWidth, defHeight);

	// Pin values
	if (defWidth > kMaxWidth)
		defWidth = kMaxWidth;
		
	if (defHeight > kMaxHeight)
		defHeight = kMaxHeight;
		
	QDesktopWidget *d = QApplication::desktop();
	if (defX < d->x() || defX > d->width())
		defX = 0;

	if (defY < d->y() + kMenuBarHeight + titleHeight || defY > d->height())
		defY = kMenuBarHeight + titleHeight;
		
	// Set up display defaults
    fLowX = 50;
	fHighX = width() - fLowX; // fLowX + width() - 10;
    fLowY = 10;
    fHighY = height() - fLowY - 22; // fLowY + width() - 22;
	
    fMaxPoints = fHighX - fLowX + 1;
    numbins = fHighY - fLowY + 1;
    
    dydv[0] = float(fHighY - fLowY) / (fHighV[0] - fLowV[0]);
    
    if (visible)
		show();
    
	// Save settings for future restore		
	SaveDimensions();		    
}


//---------------------------------------------------------------------------
// TBinChartWindow::Init
//---------------------------------------------------------------------------
void TBinChartWindow::Init()
{
	TChartWindow::Init(1);
	
    fColor[0].r = 1.0;
    fColor[0].g = 1.0;
    fColor[0].b = 1.0;
    
    fy = NULL;
    numbins = fHighY - fLowY + 1;
    myexp = 1.0;
}


//---------------------------------------------------------------------------
// TBinChartWindow::Init
//---------------------------------------------------------------------------
void TBinChartWindow::PlotPoints()
{
    for (long i = 0; i < fNumPoints[0]; i++)
        PlotPoint(fLowX + i, &fy[i * numbins]);
}


//---------------------------------------------------------------------------
// TBinChartWindow::PlotPoint
//---------------------------------------------------------------------------
void TBinChartWindow::PlotPoint(long x, const float* y)
{
	Q_CHECK_PTR(y);
	
	Color c;
    for (long i = 0; i < numbins; i++)
    {
        c.r = y[i] * fColor[0].r;
        c.g = y[i] * fColor[0].g;
        c.b = y[i] * fColor[0].b;

		glColor3fv((GLfloat *)&c.r);
		glBegin(GL_POINTS);
        	glVertex2f(x, fLowY + 1);
		glEnd();        
    }
}


//---------------------------------------------------------------------------
// TBinChartWindow::Dump
//---------------------------------------------------------------------------
void TBinChartWindow::Dump(std::ostream& out)
{
    out << fNumPoints[0] nl;
    out << numbins nl;
    for (long i = 0; i < fNumPoints[0]; i++)
    {
        for (long j = 0; j < numbins; j++)
        {
            out << fy[(i * numbins) + j] nl;
        }
    }
}


//---------------------------------------------------------------------------
// TBinChartWindow::Load
//---------------------------------------------------------------------------
void TBinChartWindow::Load(std::istream& in)
{
    in >> fNumPoints[0];
    
    if (fNumPoints[0] > fMaxPoints)
        error(2, "while loading binchartwindow from dump file, fNumPoints too big");
        
    long newnumbins;
    in >> newnumbins;
    if (newnumbins != numbins)
        error(2, "while loading binchartwindow from dump file, numbins redefined");
        
    if (y == NULL)  // first time must allocate space
    {
        y = new long[fMaxPoints*numbins];
        Q_CHECK_PTR(y);
        fy = (float*)y;
    }
    
    for (long i = 0; i < fNumPoints[0]; i++)
    {
        for (long j = 0; j < numbins; j++)
        {
            in >> fy[(i * numbins) + j];
        }
    }
}


//---------------------------------------------------------------------------
// TBinChartWindow::AddPoint
//---------------------------------------------------------------------------
void TBinChartWindow::AddPoint(const float* val, long numval)
{
    long i, j, k;
    long ymx = 0;

    if (y == NULL)  // first time must allocate space
    {
		y = new long[fMaxPoints*numbins];
		Q_CHECK_PTR(y);
        fy = (float*)y;
    }

    if (fNumPoints[0] == fMaxPoints)
    {
        // cut the number (and resolution) of points stored in half
        for (i = 1, j = 2; j < fMaxPoints; i++, j+=2)
        {
            for (k = 0; k < numbins; k++)
                fy[i * numbins + k] = fy[j * numbins +k];
        }
        fNumPoints[0] = i;
        fDecimation++;
		if( isShown() )
		{
			makeCurrent();
			Draw();
		}
    }

	long koff = fNumPoints[0] * numbins;

    for (k = 0; k < numbins; k++)
        y[koff+k] = 0;

    for (i = 0; i < numval; i++)
    {
        k = (long)((val[i] - fLowV[0]) * dydv[0]);
        k = max<long>(0, min<long>(numbins - 1, k));
        y[koff + k]++;
        ymx = std::max(ymx, y[koff + k]);
    }
		
	if( isShown() )
	{
		makeCurrent();
		PlotPoint( fNumPoints[0] + fLowX, &fy[koff] );
	}
	
	fNumPoints[0]++;
}
