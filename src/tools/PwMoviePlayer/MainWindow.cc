#include <iostream>
#include <stdio.h>

// OpenGL
#include <gl.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QGLWidget>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

// Self
#include "GLWidget.h"
#include "MainWindow.h"
#include "PwMoviePlayer.h"
#include "utils/PwMovieUtils.h"

using namespace std;

//===========================================================================
// MainWindow
//===========================================================================

//---------------------------------------------------------------------------
// MainWindow::MainWindow
//---------------------------------------------------------------------------
MainWindow::MainWindow( const char* windowTitle,
						const char* windowSettingsNameParam,
						const Qt::WindowFlags windowFlags,
						PwMovieReader* readerParam,
						char** legend,
						uint32_t startFram,
						uint32_t endFram,
						uint32_t framDelta,
						double frameRate,
						bool loop,
						bool write )
	:	QMainWindow( NULL, windowFlags )
{
	setWindowTitle( windowTitle );
	windowSettingsName = windowSettingsNameParam;
	reader = readerParam;
	startFrame = startFram == 0 ? 1 : startFram;
	endFrame = endFram == 0 ? reader->getFrameCount() : endFram;
	frameDelta = framDelta;
	looping = loop;
	writing = write;

	// Create the main menubar
	//CreateMenus( menuBar() );

	// Read the movieFile header information (version, width, height)
	// Must be done *before* the RestoreFromPrefs() and the OpenGL setup,
	// because the movie dimensions define the window dimensions for these movie windows
	//ReadMovieFileHeader();

//	printf( "movieWidth = %lu, movieHeight = %lu\n", movieWidth, movieHeight );

	// Display the main simulation window
	RestoreFromPrefs();

	QWidget *content = new QWidget( this );
	QVBoxLayout *contentLayout = new QVBoxLayout( content );

	// Set up the OpenGL view
	glWidget = new GLWidget( content, legend );

	slider = new QSlider( Qt::Horizontal, content );
	slider->setMinimum( startFrame );
	slider->setMaximum( endFrame );
	connect( slider, SIGNAL( sliderMoved(int) ), this, SLOT( sliderMoved(int) ) );

	contentLayout->addWidget( glWidget );
	contentLayout->addWidget( slider );
	content->setLayout( contentLayout );

	setCentralWidget( content );

	if( write )
		state = PLAYING;
	else
		state = PAUSED;
	SetFrame( startFrame );

	// Create playback timer
	QTimer* idleTimer = new QTimer( this );
	connect( idleTimer, SIGNAL( timeout() ), this, SLOT( Tick() ) );
	idleTimer->start( int(1000 / frameRate) );
}


//---------------------------------------------------------------------------
// MainWindow::~MainWindow
//---------------------------------------------------------------------------
MainWindow::~MainWindow()
{
	SaveWindowState();
}

//---------------------------------------------------------------------------
// MainWindow::Tick()
//---------------------------------------------------------------------------
void MainWindow::Tick()
{
	if( state == PLAYING )
	{
		NextFrame();
	}
}

//---------------------------------------------------------------------------
// MainWindow::sliderMoved()
//---------------------------------------------------------------------------
void MainWindow::sliderMoved( int value )
{
	SetFrame( (uint32_t)value );
}

//---------------------------------------------------------------------------
// MainWindow::SetFrame()
//---------------------------------------------------------------------------
void MainWindow::SetFrame( uint32_t index )
{
	if( (index >= startFrame) && (index <= endFrame) )
	{
		frame.index = index;

		reader->readFrame( frame.index,
						   &frame.timestep,
						   &frame.width,
						   &frame.height,
						   &frame.rgbBuf );

		glWidget->SetFrame( &frame );
		if( writing )
			glWidget->Write( stdout );

		slider->setSliderPosition( index );
	}
}

//---------------------------------------------------------------------------
// MainWindow::NextFrame()
//---------------------------------------------------------------------------
void MainWindow::NextFrame( uint32_t delta )
{
	if( delta == 0 )
		delta = frameDelta;
	uint32_t index = frame.index + delta;
	if( index > endFrame )
	{
		if( frame.index < endFrame )
		{
			SetFrame( endFrame );
		}
		else
		{
			if( looping )
			{
				SetFrame( startFrame );
			}
			else
			{
				if( writing )
					close();
				else
					state = PAUSED;
			}
		}
	}
	else
	{
		SetFrame( index );
	}
}

//---------------------------------------------------------------------------
// MainWindow::PrevFrame()
//---------------------------------------------------------------------------
void MainWindow::PrevFrame( uint32_t delta )
{
	if( delta == 0 )
		delta = frameDelta;
	uint32_t index = frame.index - delta;
	if( index < startFrame || index > frame.index )
		index = startFrame;
	SetFrame( index );
}

//---------------------------------------------------------------------------
// MainWindow::keyReleaseEvent
//---------------------------------------------------------------------------
void MainWindow::keyReleaseEvent( QKeyEvent* event )
{
	uint32_t delta = 0;
	if( event->modifiers() & Qt::ShiftModifier )
		delta = 1;
	else if( event->modifiers() & Qt::ControlModifier )
		delta = 10 * frameDelta;

	switch( event->key() )
	{
		case Qt::Key_Space:
			state = state == PAUSED
				? PLAYING
				: PAUSED;
			break;

		case Qt::Key_Right:
			state = PAUSED;
			NextFrame( delta );
			break;

		case Qt::Key_Left:
			state = PAUSED;
			PrevFrame( delta );
			break;

		default:
			event->ignore();
			break;
	}
}

#if 0
//---------------------------------------------------------------------------
// MainWindow::closeEvent
//---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent* ce)
{
	ce->accept();
}
#endif

//---------------------------------------------------------------------------
// MainWindow::AddFileMenu
//---------------------------------------------------------------------------
void MainWindow::CreateMenus( QMenuBar* menuBar )
{
	AddFileMenu( menuBar );
	AddHelpMenu( menuBar );
}

//---------------------------------------------------------------------------
// MainWindow::AddFileMenu
//---------------------------------------------------------------------------
void MainWindow::AddFileMenu( QMenuBar* menuBar )
{
    QMenu* menu = new QMenu( tr("&File"), this );
    menuBar->addMenu( menu );
	openAct = menu->addAction( tr("&Open..."), this, SLOT(openFile()), Qt::CTRL+Qt::Key_O );
    menu->addSeparator();
    closeAct = menu->addAction( tr("&Close"), this, SLOT(close()), Qt::CTRL+Qt::Key_W );
    quitAct = menu->addAction( tr("&Quit"), qApp, SLOT(quit()), Qt::CTRL+Qt::Key_Q );
	fileMenu = menu;
}

//---------------------------------------------------------------------------
// MainWindow::AddHelpMenu
//---------------------------------------------------------------------------
void MainWindow::AddHelpMenu( QMenuBar* menuBar )
{
	menuBar->addSeparator();	// to force to end on Motif systems (no effect on other systems)
    QMenu* menu = new QMenu( tr("&Help"), this );
    menuBar->addMenu( menu );
	aboutAct = menu->addAction( tr("&About..."), this, SLOT(about()), 0 );
    aboutQtAct = menu->addAction( tr("&About Qt..."), qApp, SLOT(aboutQt()), 0 );
	helpMenu = menu;
}

//---------------------------------------------------------------------------
// MainWindow::about
//---------------------------------------------------------------------------
void MainWindow::about()
{
   QMessageBox::about( this, tr("About Polyworld MoviePlayer"),
			tr( "<b>Polyworld MoviePlayer</b> plays digital movies recorded in Polyworld.<br>"
				"<b>Polyworld</b> evolves neural network-based artificial organisms in a computational ecology.<br>"
				"<b>Larry Yaeger</b><br>"
				"<a href=\"http://sourceforge.net/projects/polyworld/\">http://sourceforge.net/projects/polyworld/</a>" ) );
}

//---------------------------------------------------------------------------
// MainWindow::openFile
//---------------------------------------------------------------------------
void MainWindow::openFile()
{
   QMessageBox::about( this, tr("Open File..."), tr( "Not yet implemented.  Sorry." ) );
}

//---------------------------------------------------------------------------
// MainWindow::SaveWindowState
//---------------------------------------------------------------------------
void MainWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// MainWindow::SaveDimensions
//---------------------------------------------------------------------------
void MainWindow::SaveDimensions()
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
// MainWindow::SaveVisibility
//---------------------------------------------------------------------------
void MainWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );

			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}

//---------------------------------------------------------------------------
// MainWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void MainWindow::RestoreFromPrefs()
{
	QDesktopWidget* desktop = QApplication::desktop();

	// Set up some defaults
	int defX = 1;
	int titleHeight = 22;
	int defY = kMenuBarHeight + titleHeight;
// 	int defWidth = desktop->width() - defX;
// 	int defHeight = desktop->height() - defY;

	// Attempt to restore window size and position from prefs
	// Save size and location to prefs
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );

	// Window width and height are handled specially, for these movie windows,
	// since the movie dimensions fully define the window dimensions
//			if( settings.contains( "width" ) )
//				defWidth = settings.value( "width" ).toInt();
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
 	//position.setSize( QSize( defWidth, defHeight ) );
	position.setSize( QSize( 800, 600 ) );
  	setGeometry( position );
	//setFixedSize( defWidth, defHeight );
	show();

	// Save settings for future restore
	SaveWindowState();
}
