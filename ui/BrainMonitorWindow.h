//---------------------------------------------------------------------------
//	File:		BrainMonitorWindow.h
//
//	Contains:	
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef TBRAINMONITORWINDOW_H
#define TBRAINMONITORWINDOW_H

// qt
#include <qgl.h>

// Forward declarations
class critter;


//===========================================================================
// TBrainMonitorWindow
//===========================================================================
class TBrainMonitorWindow : public QGLWidget
{
public:
    TBrainMonitorWindow();
    virtual ~TBrainMonitorWindow();

	void StartMonitoring(critter* inCritter);
	void StopMonitoring();
	
	void EnableAA();
	void DisableAA();
	
	virtual void RestoreFromPrefs(long x, long y);
	
	void SaveVisibility();

protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void customEvent(QCustomEvent* event);

    void Draw();

	void SaveDimensions();
	
    critter* fCritter;
    int fPatchWidth;
    int fPatchHeight;
    
private:


};

#endif

