#pragma once

#include "MonitorView.h"

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

 private slots:
	void draw();
	void updateTarget( class AgentTracker *tracker );

 private:
	void setTracker( class AgentTracker *tracker );

 protected:
	class SceneRenderer *renderer;
	class CameraController *cameraController;
};
