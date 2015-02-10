#pragma once

// System
#include <iostream>

// Local
#include "graphics.h"
#include "MonitorView.h"

//===========================================================================
// ChartMonitorView
//===========================================================================
class ChartMonitorView : public MonitorView
{
	Q_OBJECT

public:
    ChartMonitorView( class ChartMonitor *monitor );
    virtual ~ChartMonitorView();
    
    void setRange(short ic, float valmin, float valmax);
    void setRange(float valmin, float valmax);
    
    void setColor(short ic, const Color& col);
    void setColor(const Color col);
    void setColor(short ic, float r, float g, float b);
    void setColor(float r, float g, float b);
        
    virtual void dump(std::ostream& out);
    virtual void load(std::istream& in);

public slots:
    void addPoint(short ic, float val);
    void addPoint(float val);

protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);

	void init( short ncurves, int width, int height );
	virtual void plotPoints();
	void plotPoint(short ic, long x, long y);
	void plotPoint(long x, long y);
	void drawAxes();
	void draw();
	
	long* y;  // dynamically allocated memory for storing plotted values
	long* numPoints;
	long maxPoints;
	short numCurves;
	float* lowV;
	float* highV;
	long lowY;
	long highY;
	long lowX;
	long highX;
	float* dydv;
	Color* color;
	short decimation;
};


//===========================================================================
// inlines
//===========================================================================
inline void ChartMonitorView::plotPoint(long x, long y) { plotPoint(0, x, y); }
inline void ChartMonitorView::setRange(float valmin, float valmax) { setRange(0, valmin, valmax); }
inline void ChartMonitorView::setColor(const Color col) { setColor(0, col); }
inline void ChartMonitorView::setColor(float r, float g, float b) { setColor(0, r, g, b); }
inline void ChartMonitorView::addPoint(float val) { addPoint(0,val); }
