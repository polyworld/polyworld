#pragma once

#include <QObject>


//===========================================================================
// TerminalUI
//===========================================================================
class TerminalUI : public QObject
{
	Q_OBJECT

 public:
	TerminalUI( class SimulationController *simulationController );
	virtual ~TerminalUI();

 private:
	void connectMonitors();

 private slots:
	void step();
	void status();
	void guiClosing();

 private:
	class SimulationController *simulationController;
	class StatusTextMonitor *statusTextMonitor;
	class Prompt *prompt;
	class MainWindow *gui;
};
