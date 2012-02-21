#pragma once

#include <QFont>

#include "MonitorView.h"

//===========================================================================
// StatusTextMonitorView
//===========================================================================
class StatusTextMonitorView : public MonitorView
{
	Q_OBJECT

public:
    StatusTextMonitorView( class StatusTextMonitor *monitor );
    virtual ~StatusTextMonitorView();
		
protected:
    virtual void paintGL();
    virtual void resizeGL(int width, int height);

private slots:
	void update();
        
private:
	void draw();

	class StatusTextMonitor *monitor;
	QFont font;
	int lineHeight;
	size_t nlines;
};
