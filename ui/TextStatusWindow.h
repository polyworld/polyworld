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
#include <qwidget.h>

// Forward declarations
class TSimulation;

using namespace std;

typedef vector<char *> TStatusList;


//===========================================================================
// TestStatusWindow
//===========================================================================
class TTextStatusWindow : public QWidget
{
public:
    TTextStatusWindow(TSimulation* simulation);
    virtual ~TTextStatusWindow();
    
	void RestoreFromPrefs(long x, long y);

	void SaveVisibility();

protected:
	virtual void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    
    void SaveDimensions();
        
private:
	TSimulation* fSimulation;
	bool fSaveToFile;
	
};


//===========================================================================
// inlines
//===========================================================================

#endif

	