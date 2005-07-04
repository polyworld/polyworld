//---------------------------------------------------------------------------
//	File:		TextStatusWindow.cp
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

// Self
#include "TextStatusWindow.h"

// qt
#include <qpainter.h>
#include <qsettings.h>

// Local
#include "error.h"
#include "globals.h"
#include "gscene.h"
#include "misc.h"
#include "Simulation.h"


using namespace std;

//===========================================================================
// TTextStatusWindow
//===========================================================================

//---------------------------------------------------------------------------
// TTextStatusWindow::TTextStatusWindow
//---------------------------------------------------------------------------
TTextStatusWindow::TTextStatusWindow(TSimulation* simulation)
//	:	QWidget(NULL, "TextStatusWindow", WStyle_Customize | WStyle_SysMenu | WStyle_Tool),
	:	QWidget( NULL, Qt::WindowSystemMenuHint | Qt::Tool ),
		fSimulation(simulation),
		fSaveToFile(false)
{
	setWindowTitle( "Status" );
	windowSettingsName = "Status";
}


//---------------------------------------------------------------------------
// TTextStatusWindow::~TTextStatusWindow
//---------------------------------------------------------------------------
TTextStatusWindow::~TTextStatusWindow()
{
	SaveWindowState();
}


//---------------------------------------------------------------------------
// TTextStatusWindow::paintEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::paintEvent(QPaintEvent* event)
{
	Q_ASSERT(fSimulation != NULL);
	
	// Ask simulation for a vector of status items to display
	TStatusList statusList;
	fSimulation->PopulateStatusList(statusList);
	
	// Fill background with black
	QPainter paint(this);
	QBrush brush( Qt::black );
//	paint.setBrush( brush );
	paint.fillRect( event->rect(), brush );
	
	// Draw text in white
	QFont font("LucidaGrande", 12);
	paint.setFont(font);
	paint.setPen(Qt::white);
	
	int y = 15;
			
	TStatusList::const_iterator iter = statusList.begin();
	for (; iter != statusList.end(); ++iter)
	{
		paint.drawText(7, y, *iter);
		free(*iter);
		y += 15;
	}
	
	statusList.clear();
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mousePressEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mousePressEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mouseMoveEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mouseMoveEvent(QMouseEvent* )
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mouseReleaseEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mouseReleaseEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::mouseDoubleClickEvent
//---------------------------------------------------------------------------
void TTextStatusWindow::mouseDoubleClickEvent(QMouseEvent*)
{
}


//---------------------------------------------------------------------------
// TTextStatusWindow::RestoreFromPrefs
//---------------------------------------------------------------------------
void TTextStatusWindow::RestoreFromPrefs(long x, long y)
{
//	const int kDefaultWidth = 200;
//	const int kDefaultHeight = 500;
	
	// Set up some defaults
	int defWidth = kDefaultWidth;
	int defHeight = kDefaultHeight;
	int defX = x;
	int defY = y;
	bool visible = true;
	int titleHeight = 16;
	
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
			if( settings.contains( "visible" ) )
				visible = settings.value( "visible" ).toBool();

		settings.endGroup();
	settings.endGroup();
	
	// Pin values
	if (defWidth != kDefaultWidth)
		defWidth = kDefaultWidth;
		
	if (defHeight != kDefaultHeight)
		defHeight = kDefaultHeight;
		
	if (defY < kMenuBarHeight + titleHeight)
		defY = kMenuBarHeight + titleHeight;
	
	// Set window size and location based on prefs
 	QRect position;
 	position.setTopLeft(QPoint(defX, defY));
 	position.setSize(QSize(defWidth, defHeight));
  	setGeometry(position);
	setFixedSize(defWidth, defHeight);
	
	if (visible)
		show();
	
	// Save settings for future restore		
	SaveWindowState();		
}


//---------------------------------------------------------------------------
// TTextStatusWindow::SaveWindowState
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveWindowState()
{
	SaveDimensions();
	SaveVisibility();
}


//---------------------------------------------------------------------------
// TTextStatusWindow::SaveDimensions
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveDimensions()
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
// TTextStatusWindow::SaveVisibility
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveVisibility()
{
	QSettings settings;

	settings.beginGroup( kWindowsGroupSettingsName );
		settings.beginGroup( windowSettingsName );
		
			settings.setValue( "visible", isVisible() );

		settings.endGroup();
	settings.endGroup();
}
