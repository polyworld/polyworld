// Self
#include "PWApp.h"

// QT
#include <qapplication.h>
#include <qgl.h>
#include <qlayout.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qsettings.h>
#include <qstatusbar.h>
#include <qtimer.h>

// Local
#include "critter.h"
#include "BrainMonitorWindow.h"
#include "ChartWindow.h"
#include "CritterPOVWindow.h"
#include "globals.h"
#include "SceneView.h"
#include "Simulation.h"


//===========================================================================
// main
//===========================================================================

int main(int argc, char** argv)
{
	// Create application instance
	TPWApp app(argc, argv);
	
	// It is important the we have OpenGL support
    if (!QGLFormat::hasOpenGL())
    {
		qWarning("This system has no OpenGL support. Exiting.");
		return -1;
    }
	
	// On the Mac, create the default menu bar
	QMenuBar* gGlobalMenuBar = new QMenuBar();

	// Create the main window
	TSceneWindow* appWindow = new TSceneWindow(gGlobalMenuBar);
		
	// Set up signals
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	
	// Set window as main widget and show
	app.setMainWidget(appWindow);
	
	// Create simulation timer
	QTimer* idleTimer = new QTimer(appWindow);
	app.connect(idleTimer, SIGNAL(timeout()), appWindow, SLOT(timeStep()));
	idleTimer->start(0);
	
	return app.exec();
}


//===========================================================================
// TPWApp
//===========================================================================

//---------------------------------------------------------------------------
// TPWApp::TPWApp
//---------------------------------------------------------------------------
TPWApp::TPWApp(int argc, char** argv) : QApplication(argc, argv)
{
}


//===========================================================================
// TSceneWindow
//===========================================================================

//---------------------------------------------------------------------------
// TSceneWindow::TSceneWindow
//---------------------------------------------------------------------------
TSceneWindow::TSceneWindow(QMenuBar* menuBar)
	:	QMainWindow(0, "Polyworld", WDestructiveClose | WGroupLeader),
		fMenuBar(menuBar),
		fWindowsMenu(NULL)
{	
	setMinimumSize(QSize(200, 200));

	// Create menus.  On the mac, we use the global menu created in the application
	// instance so that we have a global menu bar that spans application windows.
	// This is the default behavior on MacOS. Do not use the menuBar() method that
	// QMainWindow provides.  Use the menu member variable instead.
	AddFileMenu();    
	AddEditMenu();        
	AddWindowMenu();
	AddHelpMenu();
	        	
	// Create scene rendering view
	fSceneView = new TSceneView(this);	
	setCentralWidget(fSceneView);
	fSceneView->show();
	
	// Create the status bar at the bottom
	//QStatusBar* status = statusBar();
	//status->setSizeGripEnabled(false);

	// Create the simulation
	fSimulation = new TSimulation(fSceneView);
	fSceneView->SetSimulation(fSimulation);
		
	// Display the main simulation window
	RestoreFromPrefs();
}


//---------------------------------------------------------------------------
// TSceneWindow::~TSceneWindow
//---------------------------------------------------------------------------
TSceneWindow::~TSceneWindow()
{
	delete fSimulation;
	
	SaveDimensions();
}


//---------------------------------------------------------------------------
// TSceneWindow::closeEvent
//---------------------------------------------------------------------------
void TSceneWindow::closeEvent(QCloseEvent* ce)
{
	ce->accept();
}


#pragma mark -

//---------------------------------------------------------------------------
// TSceneWindow::AddFileMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddFileMenu()
{
    QPopupMenu* menu = new QPopupMenu(this);
    fMenuBar->insertItem("&File", menu);
	menu->insertItem("&Open...", this, SLOT(choose()), CTRL+Key_O);
	menu->insertItem("&Save", this, SLOT(save()), CTRL+Key_S);
	menu->insertItem("Save &As...", this, SLOT(saveAs()));
    menu->insertSeparator();
    menu->insertItem("&Close", this, SLOT(close()), CTRL+Key_W);
    menu->insertItem("&Quit", qApp, SLOT(closeAllWindows()), CTRL+Key_Q);
}


//---------------------------------------------------------------------------
// TSceneWindow::AddEditMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddEditMenu()
{
	// Edit menu
	QPopupMenu* edit = new QPopupMenu(this);
	fMenuBar->insertItem("&Edit", edit);
}


//---------------------------------------------------------------------------
// TSceneWindow::AddWindowMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddWindowMenu()
{
	// Window menu
	fWindowsMenu = new QPopupMenu(this);
	connect(fWindowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));
	fMenuBar->insertItem("&Window", fWindowsMenu);
	
	fWindowsMenu->insertItem("Hide Birthrate Monitor", this, SLOT(ToggleBirthrateWindow()), CTRL+Key_1, 0);
	fWindowsMenu->insertItem("Hide Fitness Monitor", this, SLOT(ToggleFitnessWindow()), CTRL+Key_2, 1);
	fWindowsMenu->insertItem("Hide Energy Monitor", this, SLOT(ToggleEnergyWindow()), CTRL+Key_3, 2);
	fWindowsMenu->insertItem("Hide Population Monitor", this, SLOT(TogglePopulationWindow()), CTRL+Key_4, 3);
	fWindowsMenu->insertItem("Hide Brain Monitor", this, SLOT(ToggleBrainWindow()), CTRL+Key_5, 4);
	fWindowsMenu->insertItem("Hide Critter POV", this, SLOT(TogglePOVWindow()), CTRL+Key_6, 5);
	fWindowsMenu->insertItem("Hide Text Status", this, SLOT(ToggleTextStatus()), CTRL+Key_7, 6);
	fWindowsMenu->insertSeparator();
	fWindowsMenu->insertItem("Tile Windows", this, SLOT(TileAllWindows()));
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleEnergyWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleEnergyWindow()
{
	Q_CHECK_PTR(fSimulation);
	TChartWindow* window = fSimulation->GetEnergyWindow();
	Q_CHECK_PTR(window);
	window->setHidden(window->isShown());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleFitnessWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleFitnessWindow()
{
	Q_CHECK_PTR(fSimulation);
	TChartWindow* window = fSimulation->GetFitnessWindow();
	Q_CHECK_PTR(window);
	window->setHidden(window->isShown());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::TogglePopulationWindow
//---------------------------------------------------------------------------
void TSceneWindow::TogglePopulationWindow()
{
	Q_CHECK_PTR(fSimulation);
	TChartWindow* window = fSimulation->GetPopulationWindow();
	Q_CHECK_PTR(window);
	window->setHidden(window->isShown());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleBirthrateWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleBirthrateWindow()
{
	Q_CHECK_PTR(fSimulation);
	TChartWindow* window = fSimulation->GetBornWindow();
	Q_CHECK_PTR(window);
	window->setHidden(window->isShown());
	window->SaveVisibility();	
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleBrainWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleBrainWindow()
{
	Q_CHECK_PTR(fSimulation);
	fSimulation->SetPrintBrain(!fSimulation->GetPrintBrain());
	TBrainMonitorWindow* window = fSimulation->GetBrainMonitorWindow();
	Q_ASSERT(window != NULL);	
	window->setHidden(fSimulation->GetPrintBrain());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::TogglePOVWindow
//---------------------------------------------------------------------------
void TSceneWindow::TogglePOVWindow()
{
	Q_ASSERT(fSimulation != NULL);
	TCritterPOVWindow* window = fSimulation->GetCritterPOVWindow();
	Q_ASSERT(window != NULL);
	window->setHidden(window->isShown());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleTextStatus
//---------------------------------------------------------------------------
void TSceneWindow::ToggleTextStatus()
{
	Q_ASSERT(fSimulation != NULL);
	TTextStatusWindow* window = fSimulation->GetStatusWindow();
	Q_ASSERT(window != NULL);
	window->setHidden(window->isShown());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::TileAllWindows
//
//
//	________________	______________________________________  __________
//	|				|	|									  | |		  |
//	|_______________|	|									  |	|		  |
//	________________	|                                     |	|		  |
//	|				|	|									  |	|         |
//	|_______________|	|									  |	|         |
//	________________	|									  |	|         |
//	|				|	|									  |	|	      |
//	|_______________|	|									  |	|_________|
//	________________	|									  |
//	|				|	|									  |
//	|_______________|	|									  |
//	________________	|									  |
//	|				|	|									  |
//	|_______________|	|_____________________________________|
//
//
//---------------------------------------------------------------------------

void TSceneWindow::TileAllWindows()
{
	Q_CHECK_PTR(fSimulation);

	const int titleHeightLarge = 22;	// frameGeometry().height() - height();
//	const int titleHeightSmall = 16;
	int nextY = kMenuBarHeight;
	
	QDesktopWidget* desktop = QApplication::desktop();
	const QRect& screenSize = desktop->screenGeometry(desktop->primaryScreen());
	const int leftX = screenSize.x();
				
	// Born
	TChartWindow* window = fSimulation->GetBornWindow();
	if (window != NULL)
	{
		window->move(leftX, nextY);
		nextY = window->frameGeometry().bottom() + 1;
	}
		
	// Fitness	
	window = fSimulation->GetFitnessWindow();
	if (window != NULL)
	{
		window->move(leftX, nextY);
		nextY = window->frameGeometry().bottom() + 1;
	}

	// Energy
	window = fSimulation->GetEnergyWindow();
	if (window != NULL)
	{
		window->move(leftX, nextY);
		nextY = window->frameGeometry().bottom() + 1;
	}

	// Population
	window = fSimulation->GetPopulationWindow();
	if (window != NULL)
	{
		window->move(leftX, nextY);
		nextY = window->frameGeometry().bottom() + 1;
	}
	
	// Gene seperation
	

	// Simulation
	move(TChartWindow::kMaxWidth + 3, kMenuBarHeight);
	resize(screenSize.width() - (300 + TChartWindow::kMaxWidth), screenSize.height() - 150);

	
	// POV (place it on top of the top part of the main Simulation window)
	TCritterPOVWindow* povWindow = fSimulation->GetCritterPOVWindow();
	int leftEdge = TChartWindow::kMaxWidth + 3;
	if (povWindow != NULL)
	{
		povWindow->move(TChartWindow::kMaxWidth + 3, kMenuBarHeight + titleHeightLarge);
		leftEdge += povWindow->width() + 3;
	}
		
	// Brain monitor
	TBrainMonitorWindow* brainWindow = fSimulation->GetBrainMonitorWindow();
	if (brainWindow != NULL)
		brainWindow->move(leftEdge, kMenuBarHeight + titleHeightLarge);

	// Text status window		
	TTextStatusWindow* statusWindow = fSimulation->GetStatusWindow();
	if (statusWindow != NULL)
		statusWindow->move(geometry().right() + 3, kMenuBarHeight);		
}


#pragma mark -

//---------------------------------------------------------------------------
// TSceneWindow::AddHelpMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddHelpMenu()
{
	// Add help item
	fMenuBar->insertSeparator();
	QPopupMenu* help = new QPopupMenu(this);
	fMenuBar->insertItem("&Help", help);
	
	// Add about item
	help->insertItem("&About", this, SLOT(about()));
	help->insertSeparator();
}


#pragma mark -


//---------------------------------------------------------------------------
// TSceneWindow::SaveDimensions
//---------------------------------------------------------------------------
void TSceneWindow::SaveDimensions()
{
	// Save size and location to prefs
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);
  
	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			settings.writeEntry("/width", geometry().width());
			settings.writeEntry("/height", geometry().height());			
			settings.writeEntry("/x", geometry().x());
			settings.writeEntry("/y", geometry().y());

		settings.endGroup();
	settings.endGroup();
}


//---------------------------------------------------------------------------
// TSceneWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TSceneWindow::RestoreFromPrefs()
{
	QDesktopWidget* desktop = QApplication::desktop();

	// Set up some defaults
	int defWidth = desktop->width();
	int defHeight = desktop->height();
	int defX = TChartWindow::kMaxWidth + 3;
	int titleHeight = 22;
	int defY = fMenuBar->height() + titleHeight;	

	
	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			defWidth = settings.readNumEntry("/width", defWidth);
			defHeight = settings.readNumEntry("/height", defHeight);
			defX = settings.readNumEntry("/x", defX);
			defY = settings.readNumEntry("/y", defY);
			
		settings.endGroup();
	settings.endGroup();
	
	// Pin values
	if (defX < desktop->x() || defX > desktop->width())
		defX = 0;

	if (defY < desktop->y() + kMenuBarHeight + titleHeight || defY > desktop->height())
		defY = kMenuBarHeight + titleHeight;

	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft(QPoint(defX, defY));
 	position.setSize(QSize(defWidth, defHeight));
  	setGeometry(position);
	setCaption(name());
	show();
	
	// Save settings for future restore		
	SaveDimensions();		
}


//---------------------------------------------------------------------------
// TSceneWindow::timeStep
//---------------------------------------------------------------------------
void TSceneWindow::timeStep()
{
	fSimulation->Step();
}


//---------------------------------------------------------------------------
// TSceneWindow::windowsMenuAboutToShow

// Set the text of the menu items based on window visibility
//---------------------------------------------------------------------------
void TSceneWindow::windowsMenuAboutToShow()
{
	Q_ASSERT(fSimulation != NULL);
	Q_ASSERT(fWindowsMenu != NULL);

	// Birthrate
	TChartWindow* window = fSimulation->GetBornWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		fWindowsMenu->setItemEnabled(0, false);	
	}
	else
	{
		if (window->isShown())
			fWindowsMenu->changeItem(0, "Hide Birthrate Monitor");
		else
			fWindowsMenu->changeItem(0, "Show Birthrate Monitor");
	}

	// Fitness
	window = fSimulation->GetFitnessWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		fWindowsMenu->setItemEnabled(1, false);	
	}
	else
	{
		if (window->isShown())
			fWindowsMenu->changeItem(1, "Hide Fitness Monitor");
		else
			fWindowsMenu->changeItem(1, "Show Fitness Monitor");
	}

	// Energy
	window = fSimulation->GetEnergyWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		fWindowsMenu->setItemEnabled(2, false);	
	}
	else
	{
		if (window->isShown())
			fWindowsMenu->changeItem(2, "Hide Energy Monitor");
		else
			fWindowsMenu->changeItem(2, "Show Energy Monitor");
	}

	// Population
	window = fSimulation->GetPopulationWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		fWindowsMenu->setItemEnabled(3, false);	
	}
	else
	{
		if (window->isShown())
			fWindowsMenu->changeItem(3, "Hide Population Monitor");
		else
			fWindowsMenu->changeItem(3, "Show Population Monitor");
	}
	
	// Brain
	TBrainMonitorWindow* brainWindow = fSimulation->GetBrainMonitorWindow();
	if (brainWindow == NULL)
	{
		// No window. Disable menu
		fWindowsMenu->setItemEnabled(4, false);	
	}
	else
	{
		if (brainWindow->isShown())
			fWindowsMenu->changeItem(4, "Hide Brain Monitor");
		else
			fWindowsMenu->changeItem(4, "Show Brain Monitor");
	}
	
	// POV
	TCritterPOVWindow* pov = fSimulation->GetCritterPOVWindow();
	if (pov == NULL)
	{
		// No window. Disable menu
		fWindowsMenu->setItemEnabled(5, false);	
	}
	else
	{
		if (pov->isShown())
			fWindowsMenu->changeItem(5, "Hide Critter POV");
		else
			fWindowsMenu->changeItem(5, "Show Critter POV");
	}
}


