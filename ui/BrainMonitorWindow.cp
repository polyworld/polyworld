//---------------------------------------------------------------------------
//	File:		BrainMonitorWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "BrainMonitorWindow.h"

// qt
#include <qapplication.h>
#include <qsettings.h>
#include <qcoreevent.h>

// Local
#include "brain.h"
#include "critter.h"
#include "globals.h"
#include "Simulation.h"

using namespace std;

//===========================================================================
// TBrainMonitorWindow
//===========================================================================
const int kMonitorCritWinWidth = 8;
const int kMonitorCritWinHeight = 8;

//---------------------------------------------------------------------------
// TBrainMonitorWindow::TBrainMonitorWindow
//---------------------------------------------------------------------------
TBrainMonitorWindow::TBrainMonitorWindow()
//	:	QGLWidget(NULL, "BrainMonitor", NULL, WStyle_Customize | WStyle_SysMenu | WStyle_Tool),
	:	QGLWidget( NULL, NULL, Qt::WindowSystemMenuHint | Qt::Tool ),
		fCritter(NULL),
		fPatchWidth(kMonitorCritWinWidth),
		fPatchHeight(kMonitorCritWinHeight)
{
	setWindowTitle( "BrainMonitor" );
	windowSettingsName = "BrainMonitor";
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::~TBrainMonitorWindow
//---------------------------------------------------------------------------
TBrainMonitorWindow::~TBrainMonitorWindow()
{
	SaveWindowState();
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::paintGL
//---------------------------------------------------------------------------
void TBrainMonitorWindow::paintGL()
{
//	Draw();
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::initializeGL
//---------------------------------------------------------------------------
void TBrainMonitorWindow::initializeGL()
{
	qglClearColor( Qt::black );
    glShadeModel( GL_SMOOTH );
//	swapBuffers();
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::resizeGL
//---------------------------------------------------------------------------
void TBrainMonitorWindow::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width, 0.0, height);	
	glMatrixMode(GL_MODELVIEW);
}
   
    
//---------------------------------------------------------------------------
// TBrainMonitorWindow::mousePressEvent
//---------------------------------------------------------------------------
void TBrainMonitorWindow::mousePressEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::mouseMoveEvent
//---------------------------------------------------------------------------
void TBrainMonitorWindow::mouseMoveEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::mouseReleaseEvent
//---------------------------------------------------------------------------
void TBrainMonitorWindow::mouseReleaseEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TBrainMonitorWindow::mouseDoubleClickEvent(QMouseEvent*)
{
}

#if 0
//---------------------------------------------------------------------------
// TBrainMonitorWindow::customEvent
//---------------------------------------------------------------------------
void TBrainMonitorWindow::customEvent( QCustomEvent* event )
{
	if( event->type() == kUpdateEventType )
		updateGL();
}
#endif

//---------------------------------------------------------------------------
// TBrainMonitorWindow::EnableAA
//---------------------------------------------------------------------------
void TBrainMonitorWindow::EnableAA()
{
	// Set up antialiasing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

//---------------------------------------------------------------------------
// TBrainMonitorWindow::DisableAA
//---------------------------------------------------------------------------
void TBrainMonitorWindow::DisableAA()
{
	// Set up antialiasing
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::Draw
//---------------------------------------------------------------------------
void TBrainMonitorWindow::Draw()
{
	if (fCritter == NULL)
		return;
	
//	printf( "isVisible() = %s\n", BoolString( isVisible() ) );
	if( !isVisible() )
		return;
	
	makeCurrent();

#if 0	
  glPushMatrix();
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width(), 0.0, height());
	glMatrixMode(GL_MODELVIEW);		
#endif

	// Clear the window
	qglClearColor( Qt::black );
	glClear( GL_COLOR_BUFFER_BIT );

	// Make sure the window is the proper size
	const long winWidth = fCritter->Brain()->GetNumNeurons()
						  * fPatchWidth + 2 * fPatchWidth;						  
	const long winHeight = fCritter->Brain()->GetNumNonInputNeurons()
					   	   * fPatchHeight + 2 * fPatchHeight;
					   	   
	if (width() != winWidth || height() != winHeight)
		setFixedSize(winWidth, winHeight);

#if 0				
	// Frame the window	
    glLineWidth(3);
	glRecti(3, 3, width() -3 , height() -3);
#endif

#if 0
	dbprintf("%s: Age = %lu, Critter = %lu, retinaBuf(0x%.8x) = (%2x,%2x,%2x,%2x) (%2x,%2x,%2x,%2x) (%2x,%2x,%2x,%2x) (%2x,%2x,%2x,%2x) ...\n",
			__FUNCTION__,
			TSimulation::fAge,
			fCritter->Number(),
			fCritter->Brain()->retinaBuf,
			fCritter->Brain()->retinaBuf[0], fCritter->Brain()->retinaBuf[1], fCritter->Brain()->retinaBuf[2], fCritter->Brain()->retinaBuf[3],
			fCritter->Brain()->retinaBuf[4], fCritter->Brain()->retinaBuf[5], fCritter->Brain()->retinaBuf[6], fCritter->Brain()->retinaBuf[7],
			fCritter->Brain()->retinaBuf[8], fCritter->Brain()->retinaBuf[9], fCritter->Brain()->retinaBuf[10], fCritter->Brain()->retinaBuf[11],
			fCritter->Brain()->retinaBuf[12], fCritter->Brain()->retinaBuf[13], fCritter->Brain()->retinaBuf[14], fCritter->Brain()->retinaBuf[15]);
#endif

	// Frame and draw the actual vision pixels
	qglColor( Qt::gray );
	glRecti( 2*fPatchWidth-1, 0, (2+brain::retinawidth)*fPatchWidth+1, fPatchHeight );
	glPixelZoom( float(fPatchWidth), float(fPatchHeight) );
	glRasterPos2i( 2*fPatchWidth, 0 );
	glDrawPixels( brain::retinawidth, 1, GL_RGBA, GL_UNSIGNED_BYTE, fCritter->Brain()->retinaBuf );
	glPixelZoom( 1.0, 1.0 );
	
	fCritter->Brain()->Render(fPatchWidth, fPatchHeight);

	swapBuffers();
//	if( visible )
//		show();
			
#if 0
  glPopMatrix();
#endif
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::StartMonitoring
//---------------------------------------------------------------------------
void TBrainMonitorWindow::StartMonitoring(critter* inCritter)
{
	if (inCritter == NULL)
		return;
		
	fCritter = inCritter;
	Q_CHECK_PTR(fCritter->Brain());
	
//	QApplication::postEvent(this, new QCustomEvent(kUpdateEventType));	
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::StopMonitoring
//---------------------------------------------------------------------------
void TBrainMonitorWindow::StopMonitoring()
{
	fCritter = NULL;
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TBrainMonitorWindow::RestoreFromPrefs(long x, long y)
{
	// Set up some defaults
	int defWidth = 500;
	int defHeight = 500;
	int defX = x;
	int defY = y;
	int titleHeight = 16;
	bool visible = false;	// only window that defaults to !visible
	
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
	if (defWidth > 500)
		defWidth = 500;
		
	if (defHeight > 500)
		defHeight = 500;
	
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
// TBrainMonitorWindow::SaveWindowState
//---------------------------------------------------------------------------
void TBrainMonitorWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TBrainMonitorWindow::SaveDimensions
//---------------------------------------------------------------------------
void TBrainMonitorWindow::SaveDimensions()
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
// TBrainMonitorWindow::SaveVisibility
//---------------------------------------------------------------------------
void TBrainMonitorWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}
