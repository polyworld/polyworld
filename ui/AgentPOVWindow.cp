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
#include <QSettings>

// Local
#include "AgentPovRenderer.h"

//===========================================================================
// TAgentPOVWindow
//===========================================================================

//---------------------------------------------------------------------------
// TAgentPOVWindow::TAgentPOVWindow
//---------------------------------------------------------------------------
TAgentPOVWindow::TAgentPOVWindow( AgentPovRenderer *renderer )
: QGLWidget( NULL, NULL, Qt::Tool )
, fRenderer( renderer )
{
	setWindowTitle( "Agent POV" );

	setFixedSize( renderer->getBufferWidth(), renderer->getBufferHeight() );

	connect( fRenderer, SIGNAL(stepRenderComplete()),
			 this, SLOT(RenderPovBuffer()) );
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::~TAgentPOVWindow
//---------------------------------------------------------------------------
TAgentPOVWindow::~TAgentPOVWindow()
{
	SaveWindowState();
}

//---------------------------------------------------------------------------
// TAgentPOVWindow::RenderPovBuffer
//---------------------------------------------------------------------------
void TAgentPOVWindow::RenderPovBuffer()
{
	if( isVisible() )
	{
		fRenderer->copyBufferTo( this );
	}
}

//---------------------------------------------------------------------------
// TAgentPOVWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TAgentPOVWindow::RestoreFromPrefs(long x, long y)
{
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;
	settings.beginGroup( "AgentPOVWindow" );

	x = settings.value( "x", (int)x ).toInt();
	y = settings.value( "y", (int)y ).toInt();
	bool visible = settings.value( "visible", true ).toBool();

	move( x, y );
	if (visible)
		show();
}


//---------------------------------------------------------------------------
// TAgentPOVWindow::SaveWindowState
//---------------------------------------------------------------------------
void TAgentPOVWindow::SaveWindowState()
{
	// Save size and location to prefs
	QSettings settings;
	settings.beginGroup( "AgentPOVWindow" );

	settings.setValue( "x", x() );
	settings.setValue( "y", y() );
	settings.setValue( "visible", isVisible() );
}
