//---------------------------------------------------------------------------
//	File:		ChartWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "SceneView.h"

// qt
#include <qapplication.h>

// Local
#include "gcamera.h"
#include "globals.h"
#include "gscene.h"
#include "Simulation.h"

using namespace std;

//===========================================================================
// TSceneView
//===========================================================================

//---------------------------------------------------------------------------
// TSceneView::TSceneView
//---------------------------------------------------------------------------
TSceneView::TSceneView( QWidget* parent )
	:	QGLWidget( parent, NULL, 0 ),
		fScene( NULL ),
		fSimulation( NULL )
{
//	setWindowTitle( "SceneView" );
	setMouseTracking( true );
#if 0
	// TEST
	// Place controller at bottom left
	fController = new TCameraController(this, &fCamera);
	QDesktopWidget* d = QApplication::desktop();
	fController->show();
	fController->move(0, (d->height() - fController->height()) - 50);
#endif	
}


//---------------------------------------------------------------------------
// TSceneView::~TSceneView
//---------------------------------------------------------------------------
TSceneView::~TSceneView()
{
}


//---------------------------------------------------------------------------
// TSceneView::SetScene
//---------------------------------------------------------------------------
void TSceneView::SetScene(gscene* scene)
{
	Q_CHECK_PTR(scene);
	fScene = scene;
}


//---------------------------------------------------------------------------
// TSceneView::SetSimulation
//---------------------------------------------------------------------------
void TSceneView::SetSimulation(TSimulation* simulation)
{
	Q_CHECK_PTR(simulation);
	fSimulation = simulation;
}


//---------------------------------------------------------------------------
// TSceneView::paintGL
//---------------------------------------------------------------------------
void TSceneView::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (fScene != NULL)
	{ 		
		glPushMatrix();
			fScene->Draw();
		glPopMatrix();
	}
}


//---------------------------------------------------------------------------
// TSceneView::initializeGL
//---------------------------------------------------------------------------
void TSceneView::initializeGL()
{
	static GLfloat pos[4] = {5.0, 5.0, 10.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    
	//glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable( GL_NORMALIZE );
}


//---------------------------------------------------------------------------
// TSceneView::resizeGL
//---------------------------------------------------------------------------
void TSceneView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
//	glTranslatef(0.0, -10.0, -40.0);
#if 0
	const long zbnear = 0x7FFFFF;
	const long zbfar = 0x0;

	glDepthRange(zbnear, zbfar);
	if (zbnear > zbfar)
		glDepthFunc(GL_EQUAL);
#endif
	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
//	gluPerspective(45.0, 1.0, 1.0, 200.0);
	GLfloat w = (float) width / (float) height;
	GLfloat h = 1.0;
	glFrustum(-w, w, -h, h, 5.0, 60.0);
}
   
    
//---------------------------------------------------------------------------
// TSceneView::mousePressEvent
//---------------------------------------------------------------------------
void TSceneView::mousePressEvent(QMouseEvent* event)
{
	// Set up dynamic panning values
	fStartMouseX = event->x();
	fStartMouseY = event->y();
	fDynamicPos = 0.0;
	fDynamicYaw = 0.0;
	fDynamicX = 0.0;
	fDynamicY = 0.0;
	fDynamicZ = 0.0;
		
	gcamera& camera = fSimulation->GetCamera();
	fCameraYaw = camera.getyaw();
	fCameraPitch = camera.getpitch();
	fCameraY = camera.y();
}


//---------------------------------------------------------------------------
// TSceneView::mouseMoveEvent
//---------------------------------------------------------------------------
void TSceneView::mouseMoveEvent(QMouseEvent* event)
{
	if( fSimulation == NULL )
		return;
		
	if( event->button() & Qt::LeftButton )
	{
		gcamera& camera = fSimulation->GetCamera();

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
	else if( event->button() & Qt::RightButton )
	{
		gcamera& camera = fSimulation->GetCamera();

		const float yawscale = 1.0;
		const float pitchscale = 1.0;

		fDynamicYaw = yawscale * (fStartMouseX - x()) * 180.0 / 640.0;
		camera.setyaw(fCameraYaw + fDynamicYaw);
		
		float dpitch = pitchscale * (fStartMouseY - y()) * 90.0 / 512.0;
		camera.setpitch(fCameraPitch + dpitch);
	}
}


//---------------------------------------------------------------------------
// TSceneView::mouseReleaseEvent
//---------------------------------------------------------------------------
void TSceneView::mouseReleaseEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TSceneView::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TSceneView::mouseDoubleClickEvent(QMouseEvent*)
{
}

#if 0
//---------------------------------------------------------------------------
// TSceneView::customEvent
//---------------------------------------------------------------------------
void TSceneView::customEvent(QCustomEvent* event)
{
	if (event->type() == kUpdateEventType)
		updateGL();
}
#endif

//---------------------------------------------------------------------------
// TSceneView::EnableAA
//---------------------------------------------------------------------------
void TSceneView::EnableAA()
{
	// Set up antialiasing
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//---------------------------------------------------------------------------
// TSceneView::DisableAA
//---------------------------------------------------------------------------
void TSceneView::DisableAA()
{
	// Set up antialiasing
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}
