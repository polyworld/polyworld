#ifndef PWAPP_H
#define PWAPP_H

// qt
#include <qapplication.h>
#include <qmainwindow.h>

// Local
#include "Simulation.h"

// Forward declarations
class QMenuBar;
class QTimer;
class TSceneView;


//===========================================================================
// TPWApp
//===========================================================================
class TPWApp : public QApplication
{
Q_OBJECT

public:
	TPWApp(int argc, char** argv);
	
};


//===========================================================================
// TSceneWindow
//===========================================================================
class TSceneWindow: public QMainWindow
{
Q_OBJECT

public:
    TSceneWindow(QMenuBar* menuBar);
    virtual ~TSceneWindow();
    
    void Update();

protected:
    void closeEvent(QCloseEvent* event);
    
	void AddFileMenu();
	void AddEditMenu();
	void AddWindowMenu();
	void AddHelpMenu();
	
	void SaveDimensions();
	void RestoreFromPrefs();
	
private slots:

	// File menu
    void choose() {}
    void save() {}
    void saveAs() {}
    void about() {}    
    void windowsMenuAboutToShow();    
    void timeStep();
    
    // Window menu
    void ToggleEnergyWindow();
    void ToggleFitnessWindow();
    void TogglePopulationWindow();
    void ToggleBirthrateWindow();
	void ToggleBrainWindow();
	void TogglePOVWindow();
	void ToggleTextStatus();
	void TileAllWindows();
    
private:
	QMenuBar* fMenuBar;
	QPopupMenu* fWindowsMenu;
	
	TSceneView* fSceneView;
	TSimulation* fSimulation;	
};

#endif


