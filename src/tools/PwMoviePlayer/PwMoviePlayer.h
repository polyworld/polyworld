#ifndef PwMoviePlayer_h
#define PwMoviePlayer_h

// System
#include <stdio.h>

// Qt
#include <QApplication>
#include <QMenuBar>

// Self
#include "MainWindow.h"

// Forward declarations

// Constants
static const char kWindowsGroupSettingsName[] = "windows";
static const int kMenuBarHeight = 22;

//===========================================================================
// PwMoviePlayer
//===========================================================================
class PMPApp : public QApplication
{
Q_OBJECT

public:
	PMPApp( int &argc, char** argv );
	~PMPApp();
	
private slots:
	void NextFrame();

private:
	MainWindow*	mainWindow;
	FILE*		mainMovieFile;
	PwMovieReader* reader;
};

#endif
