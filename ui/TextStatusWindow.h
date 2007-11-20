//---------------------------------------------------------------------------
//	File:		TextStatusWindow.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef TTEXTSTATUSWINDOW_H
#define TTEXTSTATUSWINDOW_H

// stl
#include <vector>

// qt
#include <qgl.h>

// Forward declarations
class TSimulation;

using namespace std;

typedef vector<char *> TStatusList;


//===========================================================================
// TextStatusWindow
//===========================================================================
class TTextStatusWindow : public QGLWidget
{
public:
    TTextStatusWindow(TSimulation* simulation);
    virtual ~TTextStatusWindow();
    
	void RestoreFromPrefs(long x, long y);

	void SaveVisibility();
	
	void Draw();
	
	static const int kDefaultWidth = 200;
	static const int kDefaultHeight = 500;

protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
//	virtual void paintEvent(QPaintEvent* event);
//	virtual void mousePressEvent(QMouseEvent* event);
//	virtual void mouseMoveEvent(QMouseEvent* event);
//	virtual void mouseReleaseEvent(QMouseEvent* event);
//	virtual void mouseDoubleClickEvent(QMouseEvent* event);
    
	void SaveWindowState();
    void SaveDimensions();
        
private:
	QString windowSettingsName;
	TSimulation* fSimulation;
	bool fSaveToFile;
	
};


//===========================================================================
// inlines
//===========================================================================

#endif

