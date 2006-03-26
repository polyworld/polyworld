// Self
#include "PWApp.h"
//#include "SceneView.h"

// QT
#include <qapplication.h>
#include <qgl.h>
#include <qlayout.h>
#include <qmenubar.h>
//#include <qpopupmenu.h>
#include <qmenu.h>
#include <qsettings.h>
#include <qstatusbar.h>
#include <qtimer.h>

#include <QDesktopWidget>

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
	
	// Establish how are preference settings file will be named
	QCoreApplication::setOrganizationDomain( "indiana.edu" );
//	QCoreApplication::setOrganizationName( "indiana" );
	QCoreApplication::setApplicationName( "polyworld" );
	
	// Create the main window (without passing a menubar)
	TSceneWindow* appWindow = new TSceneWindow();

	// Set up signals
	app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	
	// Set window as main widget and show
	//app.setMainWidget(appWindow);

#if 0
	// What's a sigmoid, daddy?
	for( int i = -15; i < 16; i++ )
	{
		float a = logistic( (float) i, brain::gLogisticsSlope );
		printf( "%3d  %5.2f\n", i, a );
	}
#endif

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
TPWApp::TPWApp(int& argc, char** argv) : QApplication(argc, argv)
{
}


//===========================================================================
// TSceneWindow
//===========================================================================

//---------------------------------------------------------------------------
// TSceneWindow::TSceneWindow
//---------------------------------------------------------------------------
TSceneWindow::TSceneWindow()
//	:	QMainWindow(0, "Polyworld", WDestructiveClose | WGroupLeader),
	:	QMainWindow( 0, 0 ),
		fWindowsMenu(NULL)
{
	windowSettingsName = "Polyworld";
//	setAttribute( Qt::WA_DeleteOnClose );
	
	setMinimumSize(QSize(200, 200));


	// Add menus using built-in menubar()
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

	// Display the main simulation window
	RestoreFromPrefs();

	// Set the window title, including current window dimensions
	char tempTitle[64];
	sprintf( tempTitle, "Polyworld (%d x %d)", width(), height() );
	setWindowTitle( tempTitle);

	// Create the simulation
	// NOTE: Must wait until after the above RestoreFromPrefs(), so the window
	// is the right size when we create the TSimulation object below.
	fSimulation = new TSimulation( fSceneView, this );
	fSceneView->SetSimulation( fSimulation );
}


//---------------------------------------------------------------------------
// TSceneWindow::~TSceneWindow
//---------------------------------------------------------------------------
TSceneWindow::~TSceneWindow()
{
	delete fSimulation;
	
	SaveWindowState();
}


//---------------------------------------------------------------------------
// TSceneWindow::closeEvent
//---------------------------------------------------------------------------
void TSceneWindow::closeEvent(QCloseEvent* ce)
{
	ce->accept();
}

//---------------------------------------------------------------------------
// TSceneWindow::keyPressEvent
//---------------------------------------------------------------------------
void TSceneWindow::keyReleaseEvent(QKeyEvent* event)
{
	if( fSimulation == NULL )
		return;			
	gcamera& ovCamera = fSimulation->GetOverheadCamera();
	gcamera& mCamera = fSimulation->GetCamera();
	switch (event->key())
    	{
	case Qt::Key_Equal :      		//Zoom into the simulation	  
    	case Qt::Key_Plus :      		//Zoom into the simulation	  
          if ((ovCamera.GetFOV()/1.5) >= 2.)
          {
               ovCamera.SetFOV(ovCamera.GetFOV()/1.5);                
          }
          break;
    	case Qt::Key_Minus :		       //Zoom out of the simulation	  
          if ((ovCamera.GetFOV()*1.5) < 179.)
          {
               ovCamera.SetFOV(ovCamera.GetFOV()*1.5);                
          }
      	  break;
	case Qt:: Key_R:			//Start or stop the world from turning. CMB 3/19/06
	  if(fSimulation->fRotateWorld)
	  {
	  	fSimulation->fRotateWorld = false;		
	  }
	  else
	  {
	  	fSimulation->fRotateWorld = true;
		mCamera.SetFixationPoint(0.5 * globals::worldsize, 0.0, -0.5 * globals::worldsize);	
		mCamera.SetRotation(0.0, 90, 0.0);  	
	  }
	break;
	case Qt::Key_T:	
	  //If it is enabled, disable, otherwise; re-enable (CMB 3/26/06)
	  if (fSimulation->fCritterTracking)
	  {
	  	fSimulation->fCritterTracking = false;	  
	  }
	  else
	  {
	  	fSimulation->fCritterTracking = true;	  
	  }
          if (fSimulation->fCritterTracking)
          {
          	if (!fSimulation->fOverHeadRank)
                {
                    if (fSimulation->fMonitorCritterRank)
                      fSimulation->fOverHeadRank = fSimulation->fMonitorCritterRank;
                    else
                      fSimulation->fOverHeadRank = 1;
                }
          }
          else
          {
              fSimulation->fOverHeadRank = 0;
              ovCamera.setx( 0.5*globals::worldsize);
              ovCamera.setz(-0.5*globals::worldsize);
	      ovCamera.SetFOV(90.);
          }
	  break;	
	case Qt::Key_0 :		       //Just overhead view
          fSimulation->fOverHeadRank = 0;
	  ovCamera.setx( 0.5*globals::worldsize);
          ovCamera.setz(-0.5*globals::worldsize);
	  ovCamera.SetFOV(90.);
      	  break;    	      	  	  	  	  	  
	case Qt::Key_1 :		       //Nth Fit 1
          fSimulation->fOverHeadRank = 1;
	  if (fSimulation->fMonitorCritterRank)
		fSimulation->fMonitorCritterRank = fSimulation->fOverHeadRank;
      	  break;    	      
	case Qt::Key_2 :		       //Nth Fit 2
          fSimulation->fOverHeadRank = 2;
	  if (fSimulation->fMonitorCritterRank)
		fSimulation->fMonitorCritterRank = fSimulation->fOverHeadRank;	  
      	  break;    	      
	case Qt::Key_3 :		       //Nth Fit 3
          fSimulation->fOverHeadRank = 3;
	  if (fSimulation->fMonitorCritterRank)
		fSimulation->fMonitorCritterRank = fSimulation->fOverHeadRank;	  
      	  break;    	      
	case Qt::Key_4 :		       //Nth Fit 4
          fSimulation->fOverHeadRank = 4;
	  if (fSimulation->fMonitorCritterRank)
		fSimulation->fMonitorCritterRank = fSimulation->fOverHeadRank;	  
      	  break;    	      
	case Qt::Key_5 :		       //Nth Fit 5
          fSimulation->fOverHeadRank = 5;
	  if (fSimulation->fMonitorCritterRank)
		fSimulation->fMonitorCritterRank = fSimulation->fOverHeadRank;	  
      	  break;    	      		  
    	default:
      	  event->ignore();
    	}
}

#pragma mark -

//---------------------------------------------------------------------------
// TSceneWindow::AddFileMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddFileMenu()
{
    QMenu* menu = new QMenu( "&File", this );
    menuBar()->addMenu( menu );
	menu->addAction( "&Open...", this, SLOT(choose()), Qt::CTRL+Qt::Key_O );
	menu->addAction( "&Save", this, SLOT(save()), Qt::CTRL+Qt::Key_S );
	menu->addAction( "Save &As...", this, SLOT(saveAs()));
    menu->addSeparator();
    menu->addAction( "&Close", this, SLOT(close()), Qt::CTRL+Qt::Key_W );
    menu->addAction( "&Quit", qApp, SLOT(closeAllWindows()), Qt::CTRL+Qt::Key_Q );
}


//---------------------------------------------------------------------------
// TSceneWindow::AddEditMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddEditMenu()
{
	// Edit menu
	QMenu* edit = new QMenu( "&Edit", this );
	menuBar()->addMenu( edit );
}


//---------------------------------------------------------------------------
// TSceneWindow::AddWindowMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddWindowMenu()
{
	// Window menu
	fWindowsMenu = new QMenu( "&Window", this );
	connect(fWindowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));
	menuBar()->addMenu( fWindowsMenu );
	
	toggleBirthrateWindowAct = fWindowsMenu->addAction("Hide Birthrate Monitor", this, SLOT(ToggleBirthrateWindow()), Qt::CTRL+Qt::Key_1 );
	toggleFitnessWindowAct = fWindowsMenu->addAction("Hide Fitness Monitor", this, SLOT(ToggleFitnessWindow()), Qt::CTRL+Qt::Key_2 );
	toggleEnergyWindowAct = fWindowsMenu->addAction("Hide Energy Monitor", this, SLOT(ToggleEnergyWindow()), Qt::CTRL+Qt::Key_3 );
	togglePopulationWindowAct = fWindowsMenu->addAction("Hide Population Monitor", this, SLOT(TogglePopulationWindow()), Qt::CTRL+Qt::Key_4 );
	toggleBrainWindowAct = fWindowsMenu->addAction("Hide Brain Monitor", this, SLOT(ToggleBrainWindow()), Qt::CTRL+Qt::Key_5 );
	togglePOVWindowAct = fWindowsMenu->addAction("Hide Critter POV", this, SLOT(TogglePOVWindow()), Qt::CTRL+Qt::Key_6 );
	toggleTextStatusAct = fWindowsMenu->addAction("Hide Text Status", this, SLOT(ToggleTextStatus()), Qt::CTRL+Qt::Key_7 );
	toggleOverheadWindowAct = fWindowsMenu->addAction("Hide Overhead Window", this, SLOT(ToggleOverheadWindow()), Qt::CTRL+Qt::Key_8 );
	fWindowsMenu->addSeparator();
	tileAllWindowsAct = fWindowsMenu->addAction("Tile Windows", this, SLOT(TileAllWindows()));
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleEnergyWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleEnergyWindow()
{
	Q_CHECK_PTR(fSimulation);
	TChartWindow* window = fSimulation->GetEnergyWindow();
	Q_CHECK_PTR(window);
	window->setHidden(window->isVisible());
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
	window->setHidden(window->isVisible());
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
	window->setHidden(window->isVisible());
	window->SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleBirthrateWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleBirthrateWindow()
{
	Q_CHECK_PTR(fSimulation);
	TChartWindow* window = fSimulation->GetBirthrateWindow();
	Q_CHECK_PTR(window);
	window->setHidden(window->isVisible());
	window->SaveVisibility();	
}


//---------------------------------------------------------------------------
// TSceneWindow::ToggleBrainWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleBrainWindow()
{
#if 1
	Q_CHECK_PTR(fSimulation);
	TBrainMonitorWindow* brainWindow = fSimulation->GetBrainMonitorWindow();
	Q_ASSERT(brainWindow != NULL);
//	brainWindow->visible = !brainWindow->visible;
	brainWindow->setHidden(brainWindow->isVisible());
	brainWindow->SaveVisibility();
#else
	Q_CHECK_PTR(fSimulation);
	fSimulation->SetShowBrain(!fSimulation->GetShowBrain());
	TBrainMonitorWindow* window = fSimulation->GetBrainMonitorWindow();
	Q_ASSERT(window != NULL);	
	window->setHidden(fSimulation->GetShowBrain());
	window->SaveVisibility();
#endif
}


//---------------------------------------------------------------------------
// TSceneWindow::TogglePOVWindow
//---------------------------------------------------------------------------
void TSceneWindow::TogglePOVWindow()
{
	Q_ASSERT(fSimulation != NULL);
	TCritterPOVWindow* window = fSimulation->GetCritterPOVWindow();
	Q_ASSERT(window != NULL);
	window->setHidden(window->isVisible());
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
	window->setHidden(window->isVisible());
	window->SaveVisibility();
}

//---------------------------------------------------------------------------
// TSceneWindow::ToggleOverheadWindow
//---------------------------------------------------------------------------
void TSceneWindow::ToggleOverheadWindow()
{
	Q_ASSERT(fSimulation != NULL);
	TOverheadView* window = fSimulation->GetOverheadWindow();
	Q_ASSERT(window != NULL);
	window->setHidden(window->isVisible());
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
	const int titleHeightSmall = 16;
	int nextY = kMenuBarHeight;
	
	QDesktopWidget* desktop = QApplication::desktop();
	const QRect& screenSize = desktop->screenGeometry(desktop->primaryScreen());
	const int leftX = screenSize.x();
				
	// Born
	TChartWindow* window = fSimulation->GetBirthrateWindow();
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
	
	// Gene separation
	

	// Simulation
	move( TChartWindow::kMaxWidth + 1, kMenuBarHeight );
	resize( screenSize.width() - (TChartWindow::kMaxWidth + 1), screenSize.height() );

	
	// POV (place it on top of the top part of the main Simulation window)
	TCritterPOVWindow* povWindow = fSimulation->GetCritterPOVWindow();
	int leftEdge = TChartWindow::kMaxWidth + 1;
	if( povWindow != NULL )
	{
		povWindow->move( leftEdge, kMenuBarHeight + titleHeightLarge );
	}
		
	// Brain monitor
	// (If there's plenty of room, place it beside the POV window, else
	// overlap it with the POV window, but leave the POV window title visible)
	TBrainMonitorWindow* brainWindow = fSimulation->GetBrainMonitorWindow();
	if( brainWindow != NULL )
	{
		if( (screenSize.width() - (leftEdge + povWindow->width())) > 300 )
		{
			// fair bit of room left, so place it adjacent to the POV window
			leftEdge += povWindow->width();
			nextY = kMenuBarHeight + titleHeightLarge;
		}
		else
		{
			// not much room left, so overlap it with the POV window
			nextY = kMenuBarHeight + titleHeightLarge + titleHeightSmall;
		}
		brainWindow->move( leftEdge, nextY );
	}

	// Text status window		
	TTextStatusWindow* statusWindow = fSimulation->GetStatusWindow();
	if( statusWindow != NULL )
		statusWindow->move( screenSize.width() - statusWindow->width(), kMenuBarHeight );
	
	// OverheadWindow (CMB 3/19/06
	//  Not sure really where to put it at this point.  So put it where the brain window usually goes.  Adjust later
	// (If there's plenty of room, place it beside the POV window, else
	// overlap it with the POV window, but leave the POV window title visible)
	TOverheadView* overheadWindow = fSimulation->GetOverheadWindow();
	if( overheadWindow != NULL )
	{
		if( (screenSize.width() - (leftEdge + povWindow->width())) > 300 )
		{
			// fair bit of room left, so place it adjacent to the POV window
			leftEdge += povWindow->width();
			nextY = kMenuBarHeight + titleHeightLarge;
		}
		else
		{
			// not much room left, so overlap it with the POV window
			nextY = kMenuBarHeight + titleHeightLarge + titleHeightSmall;
		}
		overheadWindow->move( leftEdge, nextY );
	}
}


#pragma mark -

//---------------------------------------------------------------------------
// TSceneWindow::AddHelpMenu
//---------------------------------------------------------------------------
void TSceneWindow::AddHelpMenu()
{
	// Add help item
//	fMenuBar->addSeparator();
	QMenu* help = new QMenu( "&Help", this );
	menuBar()->addMenu( help );
	
	// Add about item
	help->addAction("&About", this, SLOT(about()));
	help->addSeparator();
}


#pragma mark -


//---------------------------------------------------------------------------
// TSceneWindow::SaveWindowState
//---------------------------------------------------------------------------
void TSceneWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TSceneWindow::SaveDimensions
//---------------------------------------------------------------------------
void TSceneWindow::SaveDimensions()
{
	// Save size and location to prefs
	QSettings settings;
  
	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "width", geometry().width() );
			settings.setValue( "height", geometry().height() );			
			settings.setValue( "x", geometry().x() );
			settings.setValue( "y", geometry().y() );

		settings.endGroup();
	settings.endGroup();
}


//---------------------------------------------------------------------------
// TSceneWindow::SaveVisibility
//---------------------------------------------------------------------------
void TSceneWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

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
	int defX = TChartWindow::kMaxWidth + 1;
	int titleHeight = 22;
	int defY = kMenuBarHeight /*fMenuBar->height()*/ + titleHeight;	
	int defWidth = desktop->width() - defX;
	int defHeight = desktop->height() - defY;

	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			if( settings.contains( "width" ) )
				defWidth = settings.value( "width" ).toInt();
			if( settings.contains( "height" ) )
				defHeight = settings.value( "height" ).toInt();
			if( settings.contains( "x" ) )
				defX = settings.value( "x" ).toInt();
			if( settings.contains( "y" ) )
				defY = settings.value( "y" ).toInt();
			
		settings.endGroup();
	settings.endGroup();
	
	// Pin values
	if (defX < desktop->x() || defX > desktop->width())
		defX = 0;

	if (defY < desktop->y() + kMenuBarHeight + titleHeight || defY > desktop->height())
		defY = kMenuBarHeight + titleHeight;

	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft( QPoint( defX, defY ) );
 	position.setSize( QSize( defWidth, defHeight ) );
  	setGeometry( position );
	show();
	
	// Save settings for future restore		
	SaveWindowState();		
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
	TChartWindow* window = fSimulation->GetBirthrateWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		//fWindowsMenu->setItemEnabled(0, false);
		toggleBirthrateWindowAct->setEnabled( false );	
	}
	else
	{
		if (window->isVisible())
			//fWindowsMenu->changeItem(0, "Hide Birthrate Monitor");
			toggleBirthrateWindowAct->setText( "&Hide Birthrate Monitor" );
		else
			//fWindowsMenu->changeItem(0, "Show Birthrate Monitor");
			toggleBirthrateWindowAct->setText( "&Show Birthrate Monitor" );
	}

	// Fitness
	window = fSimulation->GetFitnessWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		//fWindowsMenu->setItemEnabled(1, false);	
		toggleFitnessWindowAct->setEnabled( false );
	}
	else
	{
		if (window->isVisible())
			//fWindowsMenu->changeItem(1, "Hide Fitness Monitor");
			toggleFitnessWindowAct->setText( "&Hide Fitness Monitor" );
		else
			//fWindowsMenu->changeItem(1, "Show Fitness Monitor");
			toggleFitnessWindowAct->setText( "&Show Fitness Monitor" );
	}

	// Energy
	window = fSimulation->GetEnergyWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		//fWindowsMenu->setItemEnabled(2, false);	
		toggleEnergyWindowAct->setEnabled( false );
	}
	else
	{
		if (window->isVisible())
			//fWindowsMenu->changeItem(2, "Hide Energy Monitor");
			toggleEnergyWindowAct->setText( "&Hide Energy Monitor" );
		else
			//fWindowsMenu->changeItem(2, "Show Energy Monitor");
			toggleEnergyWindowAct->setText( "&Show Energy Monitor" );
	}

	// Population
	window = fSimulation->GetPopulationWindow();
	if (window == NULL)
	{
		// No window. Disable menu
		//fWindowsMenu->setItemEnabled(3, false);
		togglePopulationWindowAct->setEnabled( false );	
	}
	else
	{
		if (window->isVisible())
			//fWindowsMenu->changeItem(3, "Hide Population Monitor");
			togglePopulationWindowAct->setText( "&Hide Population Monitor" );
		else
			//fWindowsMenu->changeItem(3, "Show Population Monitor");
			togglePopulationWindowAct->setText( "&Show Population Monitor" );
	}
	
	// Brain
	TBrainMonitorWindow* brainWindow = fSimulation->GetBrainMonitorWindow();
	if (brainWindow == NULL)
	{
		// No window. Disable menu
		//fWindowsMenu->setItemEnabled(4, false);	
		toggleBrainWindowAct->setEnabled( false );	
	}
	else
	{
		if (brainWindow->isVisible())
			//fWindowsMenu->changeItem(4, "Hide Brain Monitor");
			toggleBrainWindowAct->setText( "&Hide Brain Monitor" );
		else
			//fWindowsMenu->changeItem(4, "Show Brain Monitor");
			toggleBrainWindowAct->setText( "&Show Brain Monitor" );
	}
	
	// POV
	TCritterPOVWindow* pov = fSimulation->GetCritterPOVWindow();
	if (pov == NULL)
	{
		// No window. Disable menu
		//fWindowsMenu->setItemEnabled(5, false);	
		togglePOVWindowAct->setEnabled( false );	
	}
	else
	{
		if (pov->isVisible())
			//fWindowsMenu->changeItem(5, "Hide Critter POV");
			togglePOVWindowAct->setText( "&Hide Critter POV" );
		else
			//fWindowsMenu->changeItem(5, "Show Critter POV");
			togglePOVWindowAct->setText( "&Show Critter POV" );
	}
	
	// OverheadView
	TOverheadView* overheadWindow = fSimulation->GetOverheadWindow();
	if (overheadWindow == NULL)
	{
		// No window. Disable menu		
		toggleOverheadWindowAct->setEnabled( false );	
	}
	else
	{
		if (overheadWindow->isVisible())			
			toggleOverheadWindowAct->setText( "&Hide Overhead Window" );
		else			
			toggleOverheadWindowAct->setText( "&Show Overhead Window" );
	}
}


