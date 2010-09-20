#pragma once

#include <QWidget>

// forward decl
class QPushButton;
class QTimer;

class agent;
class BehaviorPanel;
class TSceneWindow;
class TSimulation;


void createDebugger(TSceneWindow *sceneWindow);


//===========================================================================
// DebuggerWindow
//===========================================================================

class DebuggerWindow : public QWidget
{
	Q_OBJECT

 public:
	DebuggerWindow( TSceneWindow *sceneWindow );

	virtual void closeEvent( QCloseEvent *e );

 signals:
	void timeStep();
	void agentChanged( agent *a );

 private slots:
	void execTimeStep();
	void fBtnRunPause_clicked();

 private:
	TSceneWindow *fSceneWindow;
	TSimulation *fSimulation;

	QPushButton *fBtnStep;
	QPushButton *fBtnRunPause;

	BehaviorPanel *fBehaviorPanel;

	QTimer *fTimer;
};
