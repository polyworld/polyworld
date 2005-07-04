#ifndef PWAPP_H
#define PWAPP_H

// qt
#include <qapplication.h>
#include <qmainwindow.h>
//#include <qpopupmenu.h>
#include <QMenu>

// Local
#include "Simulation.h"

// Forward declarations
class QMenuBar;
class QTimer;
class TSceneView;
class TSimulation;

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
	
	void SaveWindowState();
	void SaveDimensions();
	void SaveVisibility();
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
	QMenu* fWindowsMenu;
	
	TSceneView* fSceneView;
	TSimulation* fSimulation;
	
	QAction* toggleEnergyWindowAct;
	QAction* toggleFitnessWindowAct;
	QAction* togglePopulationWindowAct;
	QAction* toggleBirthrateWindowAct;
	QAction* toggleBrainWindowAct;
	QAction* togglePOVWindowAct;
	QAction* toggleTextStatusAct;
	QAction* tileAllWindowsAct;

	QString windowSettingsName;
};

#endif


