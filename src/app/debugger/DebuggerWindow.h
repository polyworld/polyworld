#if false

#pragma once

#include <QWidget>

// forward decl
class QPushButton;
class QTimer;

class agent;
class BehaviorPanel;
class MainWindow;
class TSimulation;


void createDebugger(MainWindow *mainWindow);


//===========================================================================
// DebuggerWindow
//===========================================================================

class DebuggerWindow : public QWidget
{
	Q_OBJECT

 public:
	DebuggerWindow( MainWindow *mainWindow );

	virtual void closeEvent( QCloseEvent *e );

 signals:
	void timeStep();
	void agentChanged( agent *a );

 private slots:
	void execTimeStep();
	void fBtnRunPause_clicked();

 private:
	MainWindow *fMainWindow;
	TSimulation *fSimulation;

	QPushButton *fBtnStep;
	QPushButton *fBtnRunPause;

	BehaviorPanel *fBehaviorPanel;

	QTimer *fTimer;
};

#endif
