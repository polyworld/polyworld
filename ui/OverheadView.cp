//---------------------------------------------------------------------------
//	File:		OverheadView.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "OverheadView.h"

// qt
#include <qapplication.h>
#include <qsettings.h>

// Local
#include "gcamera.h"
#include "globals.h"
#include "gscene.h"
#include "Simulation.h"
#include "PwMovieUtils.h"

using namespace std;

//===========================================================================
// TOverheadView
//===========================================================================

//---------------------------------------------------------------------------
// TOverheadView::TOverheadView
//---------------------------------------------------------------------------
TOverheadView::TOverheadView( TSimulation* simulation )
	:	
		QGLWidget( NULL, NULL, Qt::WindowSystemMenuHint | Qt::Tool ),		
		fScene( NULL ),
		fSimulation(simulation),
		fRecordMovie( false ),
		fMovieFile( NULL )
{
	setWindowTitle( "OverheadView" );	
	windowSettingsName = "OverheadView";
	setMouseTracking( true );	
}


//---------------------------------------------------------------------------
// TOverheadView::~TOverheadView
//---------------------------------------------------------------------------
TOverheadView::~TOverheadView()
{
}


//---------------------------------------------------------------------------
// TOverheadView::SetScene
//---------------------------------------------------------------------------
void TOverheadView::SetScene(gscene* scene)
{
	Q_CHECK_PTR(scene);
	fScene = scene;
}


//---------------------------------------------------------------------------
// TOverheadView::SetSimulation
//---------------------------------------------------------------------------
void TOverheadView::SetSimulation(TSimulation* simulation)
{
	Q_CHECK_PTR(simulation);
	fSimulation = simulation;
}


//---------------------------------------------------------------------------
// TOverheadView::Draw
//---------------------------------------------------------------------------
void TOverheadView::Draw()
{
	static unsigned long frame = 0;
	frame++;
//	printf( "%s: frame = %lu\n", __FUNCTION__, frame );
	
	makeCurrent();
	
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	if( fScene != NULL )
	{
		glPushMatrix();
			fScene->Draw();
		glPopMatrix();
	}
	else
		printf( "%s: called with fScene = NULL\n", __FUNCTION__ );

	
	swapBuffers();
	
	// Record movie to disk, if desired
	if( fRecordMovie && (frame > 1) )
		PwRecordMovie( fMovieFile, 0, 0, width(), height() );
		
}


//---------------------------------------------------------------------------
// TOverheadView::paintGL
//---------------------------------------------------------------------------
void TOverheadView::paintGL()
{
}


//---------------------------------------------------------------------------
// TOverheadView::initializeGL
//---------------------------------------------------------------------------
void TOverheadView::initializeGL()
{
	static GLfloat pos[4] = { 5.0, 5.0, 10.0, 1.0 };
    glLightfv( GL_LIGHT0, GL_POSITION, pos );
    
	//glEnable(GL_CULL_FACE);
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_NORMALIZE );
	setAutoBufferSwap( false );
}


//---------------------------------------------------------------------------
// TOverheadView::resizeGL
//---------------------------------------------------------------------------
void TOverheadView::resizeGL( int width, int height )
{
	glViewport( 0, 0, width, height );

#if 1
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	
	
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
//	glTranslatef( 0.0, -10.0, -40.0 );
#endif
#if 0
	const long zbnear = 0x7FFFFF;
	const long zbfar = 0x0;

	glDepthRange( zbnear, zbfar );
	if( zbnear > zbfar )
		glDepthFunc( GL_EQUAL );
#endif
	// Initialize projection
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
//	gluPerspective(45.0, 1.0, 1.0, 200.0);
	GLfloat w = (float) width / (float) height;
	GLfloat h = 1.0;
//	printf( "w = %g, h = %g, width = %d, height = %d\n", w, h, width, height );
	glFrustum( -w, w, -h, h, 5.0, 60.0 );
}
    
//---------------------------------------------------------------------------
// TOverheadView::mousePressEvent
//---------------------------------------------------------------------------
void TOverheadView::mousePressEvent(QMouseEvent* event)
{
	// Set up dynamic panning values
	fStartMouseX = event->x();
	fStartMouseY = event->y();
	fDynamicPos = 0.0;
	fDynamicYaw = 0.0;
	fDynamicX = 0.0;
	fDynamicY = 0.0;
	fDynamicZ = 0.0;
		
	gcamera& camera = fSimulation->GetOverheadCamera();
	fCameraYaw = camera.getyaw();
	fCameraPitch = camera.getpitch();
	fCameraY = camera.y();
	
}


//---------------------------------------------------------------------------
// TOverheadView::mouseMoveEvent
//---------------------------------------------------------------------------
void TOverheadView::mouseMoveEvent(QMouseEvent* event)
{
	if( fSimulation == NULL )
		return;			
		
	if( event->buttons() & Qt::LeftButton )		
	{
		gcamera& camera = fSimulation->GetOverheadCamera();		

		// x => dyaw, y => dpos (speed)
		const float dyawscale = 8.0;
		const float dposscale = 3.0;
		const float maxvel = 1.0;
		
		fDynamicYaw = -dyawscale * (fStartMouseX - event->x()) / 640.0;
		fDynamicPos = dposscale * (event->y() - fStartMouseY) * maxvel / 512.0;
		fDynamicX = -fDynamicPos * sin(camera.getyaw() * DEGTORAD);
		fDynamicZ = -fDynamicPos * cos(camera.getyaw() * DEGTORAD);
		
		float x = camera.x() + fDynamicX;
		float z = camera.z() + fDynamicZ;
		
		if (x < -0.1 * globals::worldsize)
		{
			if (globals::wraparound)
				x += 1.2 * globals::worldsize;
			else
				x = -0.1 * globals::worldsize;
		}			

		if (x > 1.1 * globals::worldsize)
		{
			if (globals::wraparound)
				x -= 1.2 * globals::worldsize;
			else
				x = 1.1 * globals::worldsize;
		}
		
		if (z > 0.1 * globals::worldsize)
		{
			if (globals::wraparound)
				z -= 1.2 * globals::worldsize;
			else
				z = 0.1 * globals::worldsize;
		}
		
		if (z < -1.1 * globals::worldsize)
		{
			if (globals::wraparound)
				z += 1.2 * globals::worldsize;
			else
				z = -1.1 * globals::worldsize;
		}
		
		camera.setx(x);
		camera.setz(z);
		camera.addyaw(fDynamicYaw);		
		
	}
	else if( event->buttons() & Qt::RightButton )			
	{
		gcamera& camera = fSimulation->GetOverheadCamera();
		
		const float yawscale = 1.0;
		const float pitchscale = 1.0;

		fDynamicYaw = yawscale * (fStartMouseX - x()) * 180.0 / 640.0;
		camera.setyaw(fCameraYaw + fDynamicYaw);
		
		float dpitch = pitchscale * (fStartMouseY - y()) * 90.0 / 512.0;
		camera.setpitch(fCameraPitch + dpitch);
	}
}

//---------------------------------------------------------------------------
// TOverheadView::mouseReleaseEvent
//---------------------------------------------------------------------------
void TOverheadView::mouseReleaseEvent(QMouseEvent*)
{	
}


//---------------------------------------------------------------------------
// TOverheadView::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TOverheadView::mouseDoubleClickEvent(QMouseEvent*)
{	
}

#if 0
//---------------------------------------------------------------------------
// TOverheadView::customEvent
//---------------------------------------------------------------------------
void TOverheadView::customEvent(QCustomEvent* event)
{
	if (event->type() == kUpdateEventType)
		updateGL();
}
#endif


//---------------------------------------------------------------------------
// TOverheadView::EnableAA
//---------------------------------------------------------------------------
void TOverheadView::EnableAA()
{
	// Set up antialiasing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//---------------------------------------------------------------------------
// TOverheadView::DisableAA
//---------------------------------------------------------------------------
void TOverheadView::DisableAA()
{
	// Set up antialiasing
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

//---------------------------------------------------------------------------
// TOverheadView::RestoreFromPrefs
//---------------------------------------------------------------------------
void TOverheadView::RestoreFromPrefs(long x, long y)
{
//	cout << "in RestoreFromPrefs" nl;
	
	// Set up some defaults
	int defWidth = 400;
	int defHeight = 400;
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
	setMinimumSize(400, 400);
	setMaximumSize(400, 400);
	
	if (visible)
		show();
	
	// Save settings for future restore		
	SaveWindowState();		
}


//---------------------------------------------------------------------------
// TOverheadView::SaveWindowState
//---------------------------------------------------------------------------
void TOverheadView::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TOverheadView::SaveDimensions
//---------------------------------------------------------------------------
void TOverheadView::SaveDimensions()
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
// TOverheadView::SaveVisibility
//---------------------------------------------------------------------------
void TOverheadView::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}
