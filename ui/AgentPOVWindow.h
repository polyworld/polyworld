//---------------------------------------------------------------------------
//	File:		AgentPOVWindow.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef TCHARTPOVWINDOW_H
#define TCHARTPOVWINDOW_H

// qt
#include <qgl.h>
//#include <QCustomEvent>

// Forward declarations
class TSimulation;
class agent;
//class QCustomEvent;

//===========================================================================
// TAgentPOVWindow
//===========================================================================
class TAgentPOVWindow : public QGLWidget
{
public:
    TAgentPOVWindow(int numSlots, TSimulation* simulation);
    virtual ~TAgentPOVWindow();
	
	void DrawAgentPOV( agent* c );
	void RestoreFromPrefs(long x, long y);

	void SaveVisibility();

protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
//	virtual void customEvent(QCustomEvent* event);

	void EnableAA();
	void DisableAA();

	void SaveWindowState();
    void SaveDimensions();
    
    int fSlots;
	TSimulation* fSimulation;

private:
	QString windowSettingsName;

};


//===========================================================================
// inlines
//===========================================================================

#endif

