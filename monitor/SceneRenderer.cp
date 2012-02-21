#include "SceneRenderer.h"

#include <QGLPixelBuffer>
#include <QGLWidget>

#include "CameraController.h"
#include "globals.h"
#include "PwMovieUtils.h"

//===========================================================================
// SceneRenderer
//===========================================================================

//---------------------------------------------------------------------------
// SceneRenderer::SceneRenderer
//---------------------------------------------------------------------------
SceneRenderer::SceneRenderer( gstage &stage,
							  const CameraProperties &cameraProps,
							  int _width,
							  int _height )
: width( _width )
, height( _height )
, pixelBuffer( NULL )
{
	scene.SetStage( &stage );
	scene.SetCamera( &camera );

	camera.setcolor( cameraProps.color );
	camera.SetPerspective( cameraProps.fov,
						   float(width) / float(height),
						   0.01,
						   1.5 * globals::worldsize);
}

//---------------------------------------------------------------------------
// SceneRenderer::~SceneRenderer
//---------------------------------------------------------------------------
SceneRenderer::~SceneRenderer()
{
	if( pixelBuffer )
		delete pixelBuffer;
}

//---------------------------------------------------------------------------
// SceneRenderer::getCamera
//---------------------------------------------------------------------------
gcamera &SceneRenderer::getCamera()
{
	return camera;
}

//---------------------------------------------------------------------------
// SceneRenderer::getBufferWidth
//---------------------------------------------------------------------------
int SceneRenderer::getBufferWidth()
{
	return width;
}

//---------------------------------------------------------------------------
// SceneRenderer::getBufferHeight
//---------------------------------------------------------------------------
int SceneRenderer::getBufferHeight()
{
	return height;
}

//---------------------------------------------------------------------------
// SceneRenderer::render
//---------------------------------------------------------------------------
void SceneRenderer::render()
{
	// Don't render if no slots connected
	if( receivers( SIGNAL(renderComplete()) ) == 0 )
	{
		return;
	}

	if( pixelBuffer == NULL )
	{
		pixelBuffer = new QGLPixelBuffer( width, height );
		pixelBuffer->makeCurrent();

		static GLfloat pos[4] = { 5.0, 5.0, 10.0, 1.0 };
		glLightfv( GL_LIGHT0, GL_POSITION, pos );
    
		//glEnable(GL_CULL_FACE);
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_NORMALIZE );

		glViewport( 0, 0, width, height );

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		GLfloat w = (float) width / (float) height;
		GLfloat h = 1.0;
		glFrustum( -w, w, -h, h, 5.0, 60.0 );
	}
	else
	{
		pixelBuffer->makeCurrent();
	}

	glClearColor( 0, 0, 0, 1 );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
		scene.Draw();
	glPopMatrix();

	pixelBuffer->doneCurrent();

	emit renderComplete();
}

//---------------------------------------------------------------------------
// SceneRenderer::copyTo
//---------------------------------------------------------------------------
void SceneRenderer::copyTo( QGLWidget *dst )
{
	if( pixelBuffer )
	{
		QImage image = pixelBuffer->toImage();
		QPainter painter( dst );
		painter.drawImage( QRect(0,0,dst->width(),dst->height()), image );
	}
}

//---------------------------------------------------------------------------
// SceneRenderer::createMovieRecorder
//---------------------------------------------------------------------------
PwMovieQGLPixelBufferRecorder *SceneRenderer::createMovieRecorder( PwMovieWriter *writer )
{
	assert( pixelBuffer );

	return new PwMovieQGLPixelBufferRecorder( pixelBuffer, writer );
}
