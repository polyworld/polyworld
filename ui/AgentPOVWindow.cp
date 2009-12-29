//---------------------------------------------------------------------------
//	File:		AgentPOVWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "AgentPOVWindow.h"

// qt
#include <qsettings.h>

// Local
#include "agent.h"
#include "error.h"
#include "globals.h"
#include "gscene.h"
#include "misc.h"
#include "Simulation.h"

using namespace std;

//===========================================================================
// TAgentPOVWindow
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
// TAgentPOVWindow::TAgentPOVWindow
//---------------------------------------------------------------------------
TAgentPOVWindow::TAgentPOVWindow(int numSlots, TSimulation* simulation)
//	:	QGLWidget(NULL, "AgentPOVWindow", NULL, WStyle_Customize | WStyle_SysMenu | WStyle_Tool),
	:	QGLWidget( NULL, NULL, Qt::WindowSystemMenuHint | Qt::Tool ),
		fSlots(numSlots),
		fSimulation(simulation)
{
	setWindowTitle( "Agent POV" );
	windowSettingsName = "AgentPOV";
	setAttribute( Qt::WA_MacAlwaysShowToolWindow );
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::~TAgentPOVWindow
//---------------------------------------------------------------------------
TAgentPOVWindow::~TAgentPOVWindow()
{
	SaveWindowState();
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::paintGL
//---------------------------------------------------------------------------
void TAgentPOVWindow::paintGL()
{
	// Only called once, on initialization???
	qglClearColor( Qt::black );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
	agent* c = NULL;
    //	while (agent::gXSortedAgents.next(c))
    while (agent::gXSortedAll.nextObj(AGENTTYPE, c))
		DrawAgentPOV( c );
#endif
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::DrawAgentPOV
//---------------------------------------------------------------------------
void TAgentPOVWindow::DrawAgentPOV( agent* c )
{
#if 0
	// Set up coordinate system
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif

	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Limit our drawing to this agent's small window
	glViewport( c->xleft, c->ybottom, brain::retinawidth, brain::retinaheight );

	// Do the actual drawing
	glPushMatrix();
		c->GetScene().Draw( /*c->GetFrustum()*/ );
	glPopMatrix();
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::initializeGL
//---------------------------------------------------------------------------
void TAgentPOVWindow::initializeGL()
{
//	static GLfloat pos[4] = {5.0, 5.0, 10.0, 1.0 };
//  glLightfv( GL_LIGHT0, GL_POSITION, pos );
    
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_NORMALIZE );
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::resizeGL
//---------------------------------------------------------------------------
void TAgentPOVWindow::resizeGL(int width, int height)
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
// TAgentPOVWindow::mousePressEvent
//---------------------------------------------------------------------------
void TAgentPOVWindow::mousePressEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::mouseMoveEvent
//---------------------------------------------------------------------------
void TAgentPOVWindow::mouseMoveEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::mouseReleaseEvent
//---------------------------------------------------------------------------
void TAgentPOVWindow::mouseReleaseEvent(QMouseEvent* /*event*/)
{
#if 0
	Q_ASSERT(fSimulation != NULL);
	
	// If the click is in a agent POV window, get the agent
	// that the POV represents
	
	// Determine the column and row for the click
	const int column = event->x() / kCellSize;
	const int row = event->y() / kCellSize;	
	const int index = column + (row * kMaxCellsPerRow);

	// Find agent that for the slot
	agent* c = NULL;
	agent::gXSortedAgents.reset();
	while (agent::gXSortedAgents.next(c))
	{
		if (c->Index() == index)
		{
			// Do something interesting with this information
			break;
		}
	}
	
	fSimulation->UnlockAgentList();
#endif
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TAgentPOVWindow::mouseDoubleClickEvent(QMouseEvent*)
{
}

#if 0
//---------------------------------------------------------------------------
// TAgentPOVWindow::customEvent
//---------------------------------------------------------------------------
void TAgentPOVWindow::customEvent(QCustomEvent* event)
{
	if (event->type() == kUpdateEventType)
		updateGL();
}
#endif

//---------------------------------------------------------------------------
// TAgentPOVWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TAgentPOVWindow::RestoreFromPrefs(long x, long y)
{
//	cout << "in RestoreFromPrefs" nl;
	
	// Set up some defaults
	int defWidth = agent::povwidth;
	int defHeight = agent::povheight;
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
	setMinimumSize(agent::povheight, agent::povheight);
	setMaximumSize(agent::povwidth, agent::povwidth);
	
	if (visible)
		show();
	
	// Save settings for future restore		
	SaveWindowState();		
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::EnableAA
//---------------------------------------------------------------------------
void TAgentPOVWindow::EnableAA()
{
	// Set up antialiasing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::DisableAA
//---------------------------------------------------------------------------
void TAgentPOVWindow::DisableAA()
{
	// Set up antialiasing
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::SaveWindowState
//---------------------------------------------------------------------------
void TAgentPOVWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::SaveDimensions
//---------------------------------------------------------------------------
void TAgentPOVWindow::SaveDimensions()
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
// TAgentPOVWindow::SaveVisibility
//---------------------------------------------------------------------------
void TAgentPOVWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}
