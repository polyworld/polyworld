//---------------------------------------------------------------------------
//	File:		CritterPOVWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "CritterPOVWindow.h"

// qt
#include <qsettings.h>

// Local
#include "critter.h"
#include "error.h"
#include "globals.h"
#include "gscene.h"
#include "misc.h"
#include "Simulation.h"

using namespace std;

//===========================================================================
// TCritterPOVWindow
//===========================================================================

// Individual cell dimensions
static const int kCellSize = 25;
static const int kCellPad = 2;

static const int kMaxCellsPerRow = 30;
static const int kMaxRows = 10;

// Window dimensions
static const int kMinWidth = kCellSize * kMaxCellsPerRow;
static const int kMinHeight = kCellSize * kMaxRows;
static const int kMaxWidth = kMinWidth;
static const int kMaxHeight = kMinHeight;

//---------------------------------------------------------------------------
// TCritterPOVWindow::TCritterPOVWindow
//---------------------------------------------------------------------------
TCritterPOVWindow::TCritterPOVWindow(int numSlots, TSimulation* simulation)
//	:	QGLWidget(NULL, "CritterPOVWindow", NULL, WStyle_Customize | WStyle_SysMenu | WStyle_Tool),
	:	QGLWidget( NULL, NULL, Qt::WindowSystemMenuHint | Qt::Tool ),
		fSlots(numSlots),
		fSimulation(simulation)
{
	setWindowTitle( "Critter POV" );
	windowSettingsName = "CritterPOV";
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::~TCritterPOVWindow
//---------------------------------------------------------------------------
TCritterPOVWindow::~TCritterPOVWindow()
{
	SaveWindowState();
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::paintGL
//---------------------------------------------------------------------------
void TCritterPOVWindow::paintGL()
{
	// Only called once, on initialization???
	qglClearColor( Qt::black );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
	critter* c = NULL;
    //	while (critter::gXSortedCritters.next(c))
    while (critter::gXSortedAll.nextObj(CRITTERTYPE, c))
		DrawCritterPOV( c );
#endif
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::DrawCritterPOV
//---------------------------------------------------------------------------
void TCritterPOVWindow::DrawCritterPOV( critter* c )
{
	// Set up coordinate system
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Limit our drawing to this critter's small window
	glViewport( c->xleft, c->ybottom, brain::retinawidth, brain::retinaheight );

	// Do the actual drawing
	glPushMatrix();
		c->GetScene().Draw( /*c->GetFrustum()*/ );
	glPopMatrix();
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::initializeGL
//---------------------------------------------------------------------------
void TCritterPOVWindow::initializeGL()
{
//	static GLfloat pos[4] = {5.0, 5.0, 10.0, 1.0 };
//  glLightfv( GL_LIGHT0, GL_POSITION, pos );
    
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_NORMALIZE );
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::resizeGL
//---------------------------------------------------------------------------
void TCritterPOVWindow::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
	
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
//	GLfloat w = (float) width / (float) height;
//	GLfloat h = 1.0;
//	glFrustum(-w, w, -h, h, 5.0, 60.0);
}
   
    
//---------------------------------------------------------------------------
// TCritterPOVWindow::mousePressEvent
//---------------------------------------------------------------------------
void TCritterPOVWindow::mousePressEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::mouseMoveEvent
//---------------------------------------------------------------------------
void TCritterPOVWindow::mouseMoveEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::mouseReleaseEvent
//---------------------------------------------------------------------------
void TCritterPOVWindow::mouseReleaseEvent(QMouseEvent* /*event*/)
{
#if 0
	Q_ASSERT(fSimulation != NULL);
	
	// If the click is in a critter POV window, get the critter
	// that the POV represents
	
	// Determine the column and row for the click
	const int column = event->x() / kCellSize;
	const int row = event->y() / kCellSize;	
	const int index = column + (row * kMaxCellsPerRow);

	// Find critter that for the slot
	critter* c = NULL;
	critter::gXSortedCritters.reset();
	while (critter::gXSortedCritters.next(c))
	{
		if (c->Index() == index)
		{
			// Do something interesting with this information
			break;
		}
	}
	
	fSimulation->UnlockCritterList();
#endif
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TCritterPOVWindow::mouseDoubleClickEvent(QMouseEvent*)
{
}

#if 0
//---------------------------------------------------------------------------
// TCritterPOVWindow::customEvent
//---------------------------------------------------------------------------
void TCritterPOVWindow::customEvent(QCustomEvent* event)
{
	if (event->type() == kUpdateEventType)
		updateGL();
}
#endif

//---------------------------------------------------------------------------
// TCritterPOVWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TCritterPOVWindow::RestoreFromPrefs(long x, long y)
{
//	cout << "in RestoreFromPrefs" nl;
	
	// Set up some defaults
	int defWidth = critter::povwidth;
	int defHeight = critter::povheight;
	int defX = x;
	int defY = y;
	bool visible = true;
	int titleHeight = 16;
	
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			if( settings.contains( "x" ) )
				defX = settings.value( "x" ).toInt();
			if( settings.contains( "y" ) )
				defY = settings.value( "y" ).toInt();
			if( settings.contains( "visible" ) )
				visible = settings.value( "visible" ).toBool();

		settings.endGroup();
	settings.endGroup();

	if (defY < kMenuBarHeight + titleHeight)
		defY = kMenuBarHeight + titleHeight;
	
	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft(QPoint(defX, defY));
 	position.setSize(QSize(defWidth, defHeight));
  	setGeometry(position);
	setMinimumSize(critter::povheight, critter::povheight);
	setMaximumSize(critter::povwidth, critter::povwidth);
	
	if (visible)
		show();
	
	// Save settings for future restore		
	SaveWindowState();		
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::EnableAA
//---------------------------------------------------------------------------
void TCritterPOVWindow::EnableAA()
{
	// Set up antialiasing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::DisableAA
//---------------------------------------------------------------------------
void TCritterPOVWindow::DisableAA()
{
	// Set up antialiasing
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::SaveWindowState
//---------------------------------------------------------------------------
void TCritterPOVWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::SaveDimensions
//---------------------------------------------------------------------------
void TCritterPOVWindow::SaveDimensions()
{
	// Save size and location to prefs
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "width", geometry().width() );
			settings.setValue( "height", geometry().height() );			
			settings.setValue( "x", geometry().x() );
			settings.setValue( "y", geometry().y() );

		settings.endGroup();
	settings.endGroup();
}


//---------------------------------------------------------------------------
// TCritterPOVWindow::SaveVisibility
//---------------------------------------------------------------------------
void TCritterPOVWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}
