
// Self
#include "CameraController.h"

// System
#include <gl.h>

// qt
#include <qpushbutton.h>
#include <qslider.h>
#include <qlayout.h>
#include <qframe.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qapplication.h>
#include <qkeycode.h>

// Local
#include "CameraController.h"
#include "SceneView.h"


void TCamera::RotateAxis(float delta, float x, float y, float z)
{
	glMatrixMode(GL_PROJECTION);
	glRotatef(delta, x, y, z);
}

TCameraController::TCameraController(TSceneView* scene,
									 TCamera* camera,
									 QWidget* parent,
									 const char* name)
    :	QWidget(parent, name),
    	fScene(scene),
    	fCamera(camera),
    	fLastX(0),
    	fLastY(0),
    	fLastZ(0),
    	fLastZoom(0)
{
	Q_CHECK_PTR(fScene);
	Q_CHECK_PTR(fCamera);

    QSlider* x = new QSlider(-100, 100, 60, 0, QSlider::Horizontal, this, "xsl");
    x->setTickmarks(QSlider::Below);
    connect(x, SIGNAL(valueChanged(int)), this, SLOT(setXRotation(int)));

    QSlider* y = new QSlider(-100, 100, 60, 0, QSlider::Horizontal, this, "ysl");
    y->setTickmarks(QSlider::Below);
    connect( y, SIGNAL(valueChanged(int)), this, SLOT(setYRotation(int)));

    QSlider* z = new QSlider(-100, 100, 60, 0, QSlider::Horizontal, this, "zsl");
    z->setTickmarks(QSlider::Below);
    connect(z, SIGNAL(valueChanged(int)), this, SLOT(setZRotation(int)));

    QSlider* zoom = new QSlider(-100, 100, 60, 0, QSlider::Horizontal, this, "zoomsl");
    zoom->setTickmarks(QSlider::Below);
    connect(zoom, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)));

    // Put the sliders on top of each other
    QVBoxLayout* layout = new QVBoxLayout(this, 5, 10, "vlayout");
    layout->addWidget(x);
    layout->addWidget(y);
    layout->addWidget(z);
	layout->addWidget(zoom);

}


void TCameraController::setXRotation(int value)
{
	int delta = value - fLastX;
	fScene->makeCurrent();
	fCamera->RotateAxis(delta, 1, 0, 0);
	
	fLastX = value;
}

void TCameraController::setYRotation(int value)
{
	int delta = value - fLastY;
	fScene->makeCurrent();
	fCamera->RotateAxis(delta, 0, 1, 0);
	
	fLastY = value;
}


void TCameraController::setZRotation(int value)
{
	int delta = value - fLastZ;
	fScene->makeCurrent();
	fCamera->RotateAxis(delta, 0, 0, 1);
	
	fLastZ = value;
}

void TCameraController::setZoom(int value)
{
	const float refVal = 45.0;

	int delta = value - fLastY;

	fScene->makeCurrent();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (refVal - delta, 1.0, 1.0, 200.0);
	
	fLastZoom = value;
}





