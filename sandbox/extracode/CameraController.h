#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <qwidget.h>


// Forward declarations
class TCamera;
class TSceneView;

class TCamera
{
    Q_OBJECT

public:
	TCamera();

	void RotateAxis(float delta, float x, float y, float z);
};

class TCameraController : public QWidget
{
    Q_OBJECT

public:
    TCameraController(TSceneView* scene,
    				  TCamera* camera,
    				  QWidget* parent = NULL,
    				  const char* name = NULL);

protected slots:
	void setXRotation(int value);
	void setYRotation(int value);	
	void setZRotation(int value);
	void setZoom(int value);

private:
	TSceneView* fScene;
	TCamera* fCamera;
	
	int fLastX;
	int fLastY;
	int fLastZ;
	int fLastZoom;

};


#endif

