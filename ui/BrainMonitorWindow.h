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
//#include <QCustomEvent>

// Forward declarations
class agent;
//class QCustomEvent;

//===========================================================================
// TBrainMonitorWindow
//===========================================================================
class TBrainMonitorWindow : public QGLWidget
{
public:
    TBrainMonitorWindow();
    virtual ~TBrainMonitorWindow();

	void StartMonitoring(agent* inAgent);
	void StopMonitoring();
    void Draw();
	
	void EnableAA();
	void DisableAA();
	
	virtual void RestoreFromPrefs(long x, long y);
	
	void SaveVisibility();

//	bool visible;

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
	
    agent* fAgent;
    int fPatchWidth;
    int fPatchHeight;
    
private:
	QString windowSettingsName;

};

#endif

