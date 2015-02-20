#pragma once

#include <list>

#include <QAction>
#include <QEvent>
#include <QMainWindow>

#include "MonitorView.h"


//===========================================================================
// MainWindow
//===========================================================================
class MainWindow: public QMainWindow
{
	Q_OBJECT

 public:
	MainWindow( class SimulationController *simulationController,
				bool endOnClose = true );
	virtual ~MainWindow();

 signals:
	void closing();
	
 protected:
    void closeEvent(QCloseEvent* event);    
    
 private:
	void createMonitorViews();
	void addRunMenu( QMenuBar *menuBar );
	void addViewMenu( QMenuBar *menuBar );
	
	void loadSettings();
	void saveSettings();

	bool exitOnUserConfirm();

private slots:
	void pauseOrResume();
	void endAtTimestep();
	void endNow();
    
private:
	MonitorViews monitorViews;
	class SimulationController *simulationController;
	class QAction *pausedStepAction;
	bool endOnClose;
};
