#pragma once

#include <QObject>

#include "gcamera.h"
#include "gscene.h"
#include "gstage.h"

class SceneRenderer : public QObject
{
	Q_OBJECT

 public:
	class CameraProperties
	{
	public:
		CameraProperties() : color(Color(0.3,0.3,0.3,1.0)), fov(90) {}
		CameraProperties( Color _color, float _fov ) : color(_color), fov(_fov) {}

		Color color;
		float fov;
	};

	SceneRenderer( gstage &stage,
				   const CameraProperties &cameraProps,
				   int width,
				   int height );
	virtual ~SceneRenderer();

	gcamera &getCamera();

	int getBufferWidth();
	int getBufferHeight();

	// Only renders if slots connected to renderComplete()
	void render();

	void copyTo( class QGLWidget *dst );
	class PwMovieQGLPixelBufferRecorder *createMovieRecorder( class PwMovieWriter *writer );

 signals:
	void renderComplete();

 private:
	gcamera camera;
	gscene scene;
	int width;
	int height;
	class QGLPixelBuffer *pixelBuffer;

};
