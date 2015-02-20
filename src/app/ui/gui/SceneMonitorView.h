#pragma once

#include "MonitorView.h"
#include "utils/Signal.h"

class SceneMonitorView : public MonitorView
{
	Q_OBJECT

 public:
	SceneMonitorView( class SceneMonitor *monitor );
	virtual ~SceneMonitorView();

 protected:
	virtual void showEvent( QShowEvent *event );
	virtual void hideEvent( QHideEvent *event );
	virtual void paintGL();

 private:
	void draw();
	void updateTarget( class AgentTracker *tracker );

	void setTracker( class AgentTracker *tracker );

    util::Signal<>::SlotHandle draw_handle;
    util::Signal<class AgentTracker *>::SlotHandle updateTarget_handle;
	class QtSceneRenderer *renderer;
	class CameraController *cameraController;
    class AgentTracker *tracker;
};
