#pragma once

#include "monitor/SceneRenderer.h"

class QtSceneRenderer : public SceneRenderer
{
public:
	QtSceneRenderer( gstage &stage,
                     const CameraProperties &cameraProps,
                     int width,
                     int height );
    virtual ~QtSceneRenderer();

    virtual void render() override;

	void copyTo( class QGLWidget *dst );
	class MovieRecorder *createMovieRecorder( class PwMovieWriter *writer ) override;

private:
	class QGLPixelBuffer *pixelBuffer;
};
