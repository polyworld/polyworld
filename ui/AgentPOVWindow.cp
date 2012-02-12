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
#include <QGLPixelBuffer>
#include <qsettings.h>

// Local
#include "agent.h"
#include "error.h"
#include "globals.h"
#include "gscene.h"
#include "misc.h"

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
TAgentPOVWindow::TAgentPOVWindow()
	: QGLWidget( NULL, NULL, Qt::WindowSystemMenuHint | Qt::Tool )
	, fPixelBuffer(NULL)
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
// TAgentPOVWindow::BeginStep
//---------------------------------------------------------------------------
void TAgentPOVWindow::BeginStep()
{
	if( fPixelBuffer == NULL )
	{
		fPixelBuffer = new QGLPixelBuffer( agent::povwidth, agent::povheight );
		fPixelBuffer->makeCurrent();

		glEnable( GL_DEPTH_TEST );
		glEnable( GL_NORMALIZE );

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	}
	else
	{
		fPixelBuffer->makeCurrent();
	}

	qglClearColor( Qt::black );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//---------------------------------------------------------------------------
// TAgentPOVWindow::EndStep
//---------------------------------------------------------------------------
void TAgentPOVWindow::EndStep()
{
	fPixelBuffer->doneCurrent();

	if( isVisible() )
	{
		QImage image = fPixelBuffer->toImage();
		QPainter painter( this );
		painter.drawImage( QRect(0,0,agent::povwidth,agent::povheight), image );

		swapBuffers();
	}
}

//---------------------------------------------------------------------------
// TAgentPOVWindow::DrawAgentPOV
//---------------------------------------------------------------------------
void TAgentPOVWindow::DrawAgentPOV( agent* c )
{
	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Limit our drawing to this agent's small window
	glViewport( c->xleft, c->ybottom, brain::retinawidth, brain::retinaheight );

	// Do the actual drawing
	glPushMatrix();
		c->GetScene().Draw();
	glPopMatrix();
}

//---------------------------------------------------------------------------
// TAgentPOVWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TAgentPOVWindow::RestoreFromPrefs(long x, long y)
{
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
	{
		show();
	}
	
	// Save settings for future restore		
	SaveWindowState();		
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
