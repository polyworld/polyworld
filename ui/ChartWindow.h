//---------------------------------------------------------------------------
//	File:		ChartWindow.h
//
//	Contains:	Handle drawing of objects
//
//	Copyright:
//---------------------------------------------------------------------------

#ifndef TCHARTWINDOW_H
#define TCHARTWINDOW_H


// System
#include <iostream>

// qt
#include <qgl.h>

// Local
#include "graphics.h"

//===========================================================================
// TChartWindow
//===========================================================================
class TChartWindow : public QGLWidget
{
public:
    TChartWindow(const char* name);
    TChartWindow(const char* name, short numCurves);
    virtual ~TChartWindow();
    
	void EnableAA();
	void DisableAA();

    void SetRange(short ic, float valmin, float valmax);
    void SetRange(float valmin, float valmax);
    
    void SetColor(short ic, const Color& col);
    void SetColor(const Color col);
    void SetColor(short ic, float r, float g, float b);
    void SetColor(float r, float g, float b);
    
    void AddPoint(short ic, float val);
    void AddPoint(float val);
    
    virtual void RestoreFromPrefs(long x, long y);
    
    virtual void Dump(std::ostream& out);
    virtual void Load(std::istream& in);
    
	void SaveVisibility();
    
    static int kMaxWidth;
	static int kMaxHeight;

protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void customEvent(QCustomEvent* event);

	void Init(short ncurves);
	virtual void PlotPoints();
	void PlotPoint(short ic, long x, long y);
	void PlotPoint(long x, long y);
	void DrawAxes();
	
	void SaveDimensions();
		
	long* y;  // dynamically allocated memory for storing plotted values
	long* fNumPoints;
	long fMaxPoints;
	short fNumCurves;
	float* fLowV;
	float* fHighV;
	long fLowY;
	long fHighY;
	long fLowX;
	long fHighX;
	long fTitleY;
	float* dydv;
	Color* fColor;
	short fDecimation;
    
private:


};


//===========================================================================
// inlines
//===========================================================================
inline void TChartWindow::PlotPoint(long x, long y) { PlotPoint(0, x, y); }
inline void TChartWindow::SetRange(float valmin, float valmax) { SetRange(0, valmin, valmax); }
inline void TChartWindow::SetColor(const Color col) { SetColor(0, col); }
inline void TChartWindow::SetColor(float r, float g, float b) { SetColor(0, r, g, b); }
inline void TChartWindow::AddPoint(float val) { AddPoint(0,val); }




//===========================================================================
// TBinChartWindow
//===========================================================================
class TBinChartWindow : public TChartWindow
{
public:
    TBinChartWindow(const char* name);
        
    void SetExponent(float e);
    float GetExponent();
    
    void AddPoint(const float* val, long numval);
    
    virtual void RestoreFromPrefs(long x, long y);
        
    virtual void Dump(std::ostream& out);
    virtual void Load(std::istream& in);

protected:
	virtual void Init();
	virtual void PlotPoints();
	void PlotPoint(long x, const float* y);

	long numbins;
	float* fy;
	float myexp;
};


//===========================================================================
// inlines
//===========================================================================
inline void TBinChartWindow::SetExponent(float e) { myexp = e; }
inline float TBinChartWindow::GetExponent() { return myexp; }


#endif

