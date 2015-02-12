#pragma once

#include "MonitorView.h"
#include "Signal.h"

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

private:
	void updateTarget( class AgentTracker * );
    util::Signal<class AgentTracker *>::SlotHandle updateTarget_handle;

private slots:
    void draw();
};

