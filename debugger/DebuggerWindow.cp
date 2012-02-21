#if false

#include "DebuggerWindow.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "BehaviorPanel.h"
#include "GenomeUtil.h"
#include "MainWindow.h"
#include "Simulation.h"

using namespace genome;

void createDebugger(MainWindow *mainWindow)
{
	DebuggerWindow *debugger = new DebuggerWindow(mainWindow);
	debugger->show();
}

//---------------------------------------------------------------------------
// DebuggerWindow::DebuggerWindow
//---------------------------------------------------------------------------
DebuggerWindow::DebuggerWindow( MainWindow *mainWindow ) : QWidget( NULL )
{
	fMainWindow = mainWindow;
	fSimulation = fMainWindow->GetSimulation();

	setWindowTitle( "PW Debugger" );

	QVBoxLayout *layout = new QVBoxLayout( this );
	setLayout( layout );

	//
	// Run Buttons
	// 
	QWidget *runPanel = new QWidget( this );
	QHBoxLayout *runPanelLayout = new QHBoxLayout( runPanel );
	runPanel->setLayout( runPanelLayout );

	fBtnStep = new QPushButton(tr("Step"));
	connect(fBtnStep, SIGNAL(clicked()), this, SLOT(execTimeStep()));
	runPanelLayout->addWidget(fBtnStep);

	fBtnRunPause = new QPushButton(tr("Run"));
	connect(fBtnRunPause, SIGNAL(clicked()), this, SLOT(fBtnRunPause_clicked()));
	runPanelLayout->addWidget(fBtnRunPause);

	layout->addWidget( runPanel );

	//
	// Behavior Panel
	//
	fBehaviorPanel = new BehaviorPanel( this,
										GenomeUtil::schema,
										this );
	layout->addWidget( fBehaviorPanel );

	//
	// Non-GUI
	//
	fTimer = new QTimer(this);
	connect(fTimer, SIGNAL(timeout()), this, SLOT(execTimeStep()));
}

//---------------------------------------------------------------------------
// DebuggerWindow::closeEvent
//---------------------------------------------------------------------------
void DebuggerWindow::closeEvent( QCloseEvent *e )
{
	exit( 0 );
}

//---------------------------------------------------------------------------
// DebuggerWindow::execTimeStep
//---------------------------------------------------------------------------
void DebuggerWindow::execTimeStep()
{
	fMainWindow->timeStep();

	if( fSimulation->fStep == 1 )
	{
		objectxsortedlist::gXSortedObjects.reset();
		agent *a;

		objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject **)&a );

		agentChanged( a );
	}

	timeStep();
}

//---------------------------------------------------------------------------
// DebuggerWindow::fBtnRunPause_clicked
//---------------------------------------------------------------------------
void DebuggerWindow::fBtnRunPause_clicked()
{
	if( fBtnRunPause->text() == "Run" )
	{
		//
		// User Clicked "Run"
		// 

		fBtnRunPause->setText( "Pause" );
		fBtnStep->setEnabled( false );

		fTimer->start(0);
	}
	else
	{
		//
		// User Clicked "Pause"
		// 

		fBtnRunPause->setText( "Run" );
		fBtnStep->setEnabled( true );

		fTimer->stop();
	}
}

#endif
