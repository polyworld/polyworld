#pragma once

#include "MonitorView.h"

//===========================================================================
// PovMonitorView
//===========================================================================
class PovMonitorView : public MonitorView
{
	Q_OBJECT

 public:
	PovMonitorView( class PovMonitor *monitor );
    virtual ~PovMonitorView();
	
 protected:
	virtual void paintGL();

 private slots:
	void draw();

 private:
	class AgentPovRenderer *renderer;
};
