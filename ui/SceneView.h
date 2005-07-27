//---------------------------------------------------------------------------
//	File:		SceneView.h
//
//	Contains:	
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef SCENEVIEW_H
#define SCENEVIEW_H


// qt
#include <qgl.h>
//#include <QCustomEvent>

// Forware declarations
class gscene;
class TSimulation;
//class QCustomEvent;

//===========================================================================
// TSceneView
//===========================================================================
class TSceneView : public QGLWidget
{
public:
    TSceneView(QWidget* parent);
    virtual ~TSceneView();
    
    void SetScene(gscene* scene);
    void SetSimulation(TSimulation* simulation);
	void Draw();
    
	void EnableAA();
	void DisableAA();
	
	void setRecordMovie( bool recordMovie, FILE* movieFile ) { fRecordMovie = recordMovie; fMovieFile = movieFile; }
	
protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
//	virtual void customEvent(QCustomEvent* event);
	
private:
	gscene* fScene;
	TSimulation* fSimulation;
	
	int fStartMouseX;
	int fStartMouseY;
	float fDynamicPos;
	float fDynamicYaw;
	float fDynamicX;
	float fDynamicY;
	float fDynamicZ;
	float fCameraYaw;
	float fCameraPitch;
	float fCameraY;
	
	bool fRecordMovie;
	FILE* fMovieFile;
};


//===========================================================================
// inlines
//===========================================================================

#endif

