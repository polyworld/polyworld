//---------------------------------------------------------------------------
//	File:		CritterPOVWindow.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef TCHARTPOVWINDOW_H
#define TCHARTPOVWINDOW_H

// qt
#include <qgl.h>

// Forward declarations
class TSimulation;
class critter;

//===========================================================================
// TCritterPOVWindow
//===========================================================================
class TCritterPOVWindow : public QGLWidget
{
public:
    TCritterPOVWindow(int numSlots, TSimulation* simulation);
    virtual ~TCritterPOVWindow();
	
	void DrawCritterPOV( critter* c );
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
	virtual void customEvent(QCustomEvent* event);

	void EnableAA();
	void DisableAA();

    void SaveDimensions();
    
    int fSlots;
	TSimulation* fSimulation;

private:


};


//===========================================================================
// inlines
//===========================================================================

#endif

