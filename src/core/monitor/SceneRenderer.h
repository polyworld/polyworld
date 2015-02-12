#pragma once

#include "graphics/gcamera.h"
#include "graphics/gscene.h"
#include "graphics/gstage.h"
#include "utils/Signal.h"

class SceneRenderer
{
 public:
	class CameraProperties
	{
	public:
		CameraProperties() : color(Color(0.3,0.3,0.3,1.0)), fov(90) {}
		CameraProperties( Color _color, float _fov ) : color(_color), fov(_fov) {}

		Color color;
		float fov;
	};

    static SceneRenderer *create(gstage &stage,
                                 const CameraProperties &cameraProps,
                                 int width,
                                 int height);

	virtual ~SceneRenderer();

	gcamera &getCamera();

	int getBufferWidth();
	int getBufferHeight();

    util::Signal<> renderComplete;

    virtual class MovieRecorder *createMovieRecorder(class PwMovieWriter *writer) = 0;
	// Only renders if slots connected to renderComplete()
	virtual void render() = 0;

 protected:
	SceneRenderer( gstage &stage,
				   const CameraProperties &cameraProps,
				   int width,
				   int height );

	gcamera camera;
	gscene scene;
	int width;
	int height;
};
