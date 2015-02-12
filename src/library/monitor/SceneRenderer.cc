#include "SceneRenderer.h"

#include "CameraController.h"
#include "globals.h"

//===========================================================================
// SceneRenderer
//===========================================================================

//---------------------------------------------------------------------------
// SceneRenderer::SceneRenderer
//---------------------------------------------------------------------------
SceneRenderer::SceneRenderer( gstage &stage,
							  const CameraProperties &cameraProps,
							  int width_,
							  int height_ )
: width( width_ )
, height( height_ )
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
