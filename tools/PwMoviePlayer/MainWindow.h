#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define PW_USE_QMAINWINDOW 1

#include <stdint.h>

#include <QGLWidget>
#include <QKeyEvent>
#include <QMainWindow>
#include <QSlider>

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
	MainWindow( const char* windowTitle,
				const char* windowSettingsNameParam,
				const Qt::WindowFlags windowFlags,
				PwMovieReader* reader,
				char** legend,
				uint32_t startFram,
				uint32_t endFram,
				double frameRate,
				bool loop);
	~MainWindow();

protected:
	virtual void keyReleaseEvent( QKeyEvent* event );

private slots:
	void Tick();
	void sliderMoved( int value );
	void openFile();
	void about();
//	void aboutQt();

private:
	uint32_t startFrame;
	uint32_t endFrame;
	bool looping;

	void SetFrame( uint32_t index );
	void NextFrame();
	void PrevFrame();

	void CreateMenus( QMenuBar* menuBar );
	void AddFileMenu( QMenuBar* menuBar );
	void AddHelpMenu( QMenuBar* menuBar );

	void RestoreFromPrefs();
	void SaveWindowState();
	void SaveDimensions();
	void SaveVisibility();

	GLWidget*	glWidget;
	GLWidget::Frame frame;

	QSlider* slider;

	enum State
	{
		STOPPED,
		PLAYING,
		PAUSED
	} state;

	PwMovieReader* reader;

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
