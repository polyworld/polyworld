#pragma once

#include <list>

#include <QGLWidget>

class MonitorView : public QGLWidget
{
 public:
	MonitorView( class Monitor *monitor,
				 int defaultWidth = -1,
				 int defaultHeight = -1,
				 bool fixedSize = true );
	virtual ~MonitorView();

	virtual class Monitor *getMonitor();

	virtual void loadSettings();
	virtual void saveSettings();

 private:
	class Monitor *monitor;
	int defaultWidth;
	int defaultHeight;
	bool fixedSize;
};

typedef std::list<MonitorView *> MonitorViews;
