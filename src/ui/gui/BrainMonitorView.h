#pragma once

#include "MonitorView.h"

//===========================================================================
// BrainMonitorView
//===========================================================================
class BrainMonitorView : public MonitorView
{
	Q_OBJECT

public:
    BrainMonitorView( class BrainMonitor *monitor );
    virtual ~BrainMonitorView();

protected:
	virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);

    class agent* fAgent;

private slots:
    void draw();
	void updateTarget( class AgentTracker * );
};

