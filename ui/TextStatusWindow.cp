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
	:	QWidget(NULL, "TextStatusWindow", WStyle_Customize | WStyle_SysMenu | WStyle_Tool),
		fSimulation(simulation),
		fSaveToFile(false)
{
	setCaption("Status");
}


//---------------------------------------------------------------------------
// TTextStatusWindow::~TTextStatusWindow
//---------------------------------------------------------------------------
TTextStatusWindow::~TTextStatusWindow()
{
	SaveDimensions();
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
	paint.setBackgroundColor(Qt::black);
	paint.fillRect(event->rect(), Qt::black);
	
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
	const int kDefaultWidth = 200;
	const int kDefaultHeight = 500;
	
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
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			defWidth = settings.readNumEntry("/width", defWidth);
			defHeight = settings.readNumEntry("/height", defHeight);
			defX = settings.readNumEntry("/x", defX);
			defY = settings.readNumEntry("/y", defY);
			visible = settings.readBoolEntry("/visible", visible);

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
	SaveDimensions();		
}


//---------------------------------------------------------------------------
// TTextStatusWindow::SaveDimensions
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveDimensions()
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
// TTextStatusWindow::SaveVisibility
//---------------------------------------------------------------------------
void TTextStatusWindow::SaveVisibility()
{
	QSettings settings;
	settings.setPath(kPrefPath, kPrefSection);

	settings.beginGroup("/windows");
		settings.beginGroup(name());
		
			settings.writeEntry("/visible", isShown());

		settings.endGroup();
	settings.endGroup();
}
