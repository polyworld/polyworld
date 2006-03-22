//---------------------------------------------------------------------------
//	File:		OverheadView.h
//
//	Contains:	
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef TOVERHEADWINDOW_H
#define TOVERHEADWINDOW_H

// unix
#include <stdio.h>

// qt
#include <qgl.h>
//#include <QCustomEvent>
// Forware declarations
class gscene;
class TSimulation;
//class QCustomEvent;

//===========================================================================
// TOverheadView
//===========================================================================
class TOverheadView : public QGLWidget
{
public:
    TOverheadView(TSimulation* simulation);
    virtual ~TOverheadView();
    
    void SetScene(gscene* scene);
    void SetSimulation(TSimulation* simulation);
	void Draw();
    
	void EnableAA();
	void DisableAA();
	
	void RestoreFromPrefs(long x, long y);
	void SaveVisibility();
	
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

    void SaveWindowState();
    void SaveDimensions();
	
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
	QString windowSettingsName;
};


//===========================================================================
// inlines
//===========================================================================

#endif

