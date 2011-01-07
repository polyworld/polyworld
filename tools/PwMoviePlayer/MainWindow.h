#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define PW_USE_QMAINWINDOW 1

#include <stdint.h>

#include <QGLWidget>
#include <QKeyEvent>
#include <QMainWindow>

#include "GLWidget.h"

class QAction;
class QActionGroup;
class QLabel;
class QMenu;

#if PW_USE_QMAINWINDOW
class MainWindow : public QMainWindow
#else
class MainWindow : public QWidget
#endif
{
	Q_OBJECT

public:
	MainWindow( const char* windowTitle, const char* windowSettingsNameParam, const Qt::WFlags windowFlags,
			    FILE* movieFileParam, PwMovieIndexer* indexer,
				char** legend,
				uint32_t startFrame, uint32_t endFrame, double frameRate );
	~MainWindow();
	
	void NextFrame();

protected:
	virtual void keyReleaseEvent( QKeyEvent* event );

private slots:
	void openFile();
	void about();
//	void aboutQt();

private:
	void CreateMenus( QMenuBar* menuBar );
	void AddFileMenu( QMenuBar* menuBar );
	void AddHelpMenu( QMenuBar* menuBar );
	
	void ReadMovieFileHeader();
	void RestoreFromPrefs();
	void SaveWindowState();
	void SaveDimensions();
	void SaveVisibility();
	
	GLWidget*	glWidget;
	
	bool		paused;
	bool		step;
	bool        prev;
	
	FILE*		movieFile;
	PwMovieIndexer* indexer;
	uint32_t		movieVersion;
	uint32_t		movieWidth;
	uint32_t		movieHeight;
	
	QMenu*		fileMenu;
	QMenu*		helpMenu;

	QAction*	openAct;
	QAction*	closeAct;
	QAction*	quitAct;
	QAction*	aboutAct;
	QAction*	aboutQtAct;

	QString windowSettingsName;
};

#endif
