//---------------------------------------------------------------------------
//	File:		TextStatusWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "TextStatusWindow.h"

// qt
#include <qpainter.h>
#include <qsettings.h>

// Local
#include "error.h"
#include "globals.h"
#include "gscene.h"
#include "misc.h"
#include "Simulation.h"


using namespace std;

//===========================================================================
// TTextStatusWindow
//===========================================================================

//---------------------------------------------------------------------------
// TTextStatusWindow::TTextStatusWindow
//---------------------------------------------------------------------------
TTextStatusWindow::TTextStatusWindow(TSimulation* simulation)
//	:	QWidget(NULL, "TextStatusWindow", WStyle_Customize | WStyle_SysMenu | WStyle_Tool),
	:	QGLWidget( NULL, NULL, Qt::WindowSystemMenuHint | Qt::Tool ),
		fSimulation(simulation),
		fSaveToFile(false)
{
	setWindowTitle( "Status" );
	windowSettingsName = "Status";
}


//---------------------------------------------------------------------------
// TTextStatusWindow::~TTextStatusWindow
//---------------------------------------------------------------------------
TTextStatusWindow::~TTextStatusWindow()
{
	SaveWindowState();
}


//---------------------------------------------------------------------------
// TTextStatusWindow::initializeGL
//---------------------------------------------------------------------------
void TTextStatusWindow::initializeGL()
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::paintGL
//---------------------------------------------------------------------------
void TTextStatusWindow::paintGL()
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::resizeGL
//---------------------------------------------------------------------------
void TTextStatusWindow::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width, 0.0, height);	
	glMatrixMode(GL_MODELVIEW);
}


//---------------------------------------------------------------------------
// TTextStatusWindow::Draw
//---------------------------------------------------------------------------
#define TextStatusLineHeight 14
void TTextStatusWindow::Draw()
{
	Q_ASSERT(fSimulation != NULL);
	
	TStatusList::const_iterator iter;
	
	// Ask simulation for a vector of status items to display
	TStatusList statusList;
	fSimulation->PopulateStatusList(statusList);
	
	// TODO: The following test doesn't work as hoped.  I'd like to avoid
	// making the window current and drawing into it if the app isn't in
	// the foreground.  (But we want to continue calling PopulateStatusList(),
	// because it also prints to our stat file, when appropriate.)
	if( !updatesEnabled() )
	{
		// If we aren't updating the window display, just free the strings
		iter = statusList.begin();
		for (; iter != statusList.end(); ++iter)
			free(*iter);
		statusList.clear();
		
		return;
	}
	
	makeCurrent();
	
 	QRect geom = geometry();
	int newHeight = TextStatusLineHeight * statusList.size()  +  3;
	if( newHeight != geom.height() )
	{
		geom.setHeight( newHeight );
		setGeometry( geom );
		setFixedSize( geom.width(), newHeight );	
	}
	
	// Clear the window to black
	qglClearColor( Qt::black );
	glClear( GL_COLOR_BUFFER_BIT );
	
	// Draw text in white
//	QFont font("LucidaGrande", 12, QFont::DemiBold);
//	QFont font("Andale Mono", 12, QFont::Normal);
	QFont font("Futura Condensed Medium", 12, QFont::Normal);
//	QFont font( "Monaco", 9 );
//	QFont font("Courier", 10, QFont::Normal);
//	font.setStyleHint( QFont::AnyStyle, QFont::OpenGLCompatible );
    glColor4ub( 255, 255, 255, 255 );
	
	int y = TextStatusLineHeight;
			
	iter = statusList.begin();
	for (; iter != statusList.end(); ++iter)
	{
		renderText( 7, y, *iter, font );
		free(*iter);
		y += TextStatusLineHeight;
	}
	
	statusList.clear();
	
	swapBuffers();
}

#if 0
//---------------------------------------------------------------------------
// TTextStatusWindow::mousePressEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mousePressEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mouseMoveEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mouseMoveEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mouseReleaseEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mouseReleaseEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mouseDoubleClickEvent(QMouseEvent*)
{
}
#endif

//---------------------------------------------------------------------------
// TTextStatusWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TTextStatusWindow::RestoreFromPrefs(long x, long y)
{
//	const int kDefaultWidth = 200;
//	const int kDefaultHeight = 500;
	
	// Set up some defaults
	int defWidth = kDefaultWidth;
	int defHeight = kDefaultHeight;
	int defX = x;
	int defY = y;
	bool visible = true;
	int titleHeight = 16;
	
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			if( settings.contains( "width" ) )
				defWidth = settings.value( "width" ).toInt();
			if( settings.contains( "height" ) )
				defHeight = settings.value( "height" ).toInt();
			if( settings.contains( "x" ) )
				defX = settings.value( "x" ).toInt();
			if( settings.contains( "y" ) )
				defY = settings.value( "y" ).toInt();
			if( settings.contains( "visible" ) )
				visible = settings.value( "visible" ).toBool();

		settings.endGroup();
	settings.endGroup();
	
	// Pin values
	if (defWidth != kDefaultWidth)
		defWidth = kDefaultWidth;
		
	if (defHeight != kDefaultHeight)
		defHeight = kDefaultHeight;
		
	if (defY < kMenuBarHeight + titleHeight)
		defY = kMenuBarHeight + titleHeight;
	
	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft(QPoint(defX, defY));
 	position.setSize(QSize(defWidth, defHeight));
  	setGeometry(position);
	setFixedSize(defWidth, defHeight);
	
	if (visible)
		show();
	
	// Save settings for future restore		
	SaveWindowState();		
}


//---------------------------------------------------------------------------
// TTextStatusWindow::SaveWindowState
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TTextStatusWindow::SaveDimensions
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveDimensions()
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
// TTextStatusWindow::SaveVisibility
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}
