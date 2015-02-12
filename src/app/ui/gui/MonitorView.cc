#include "MonitorView.h"

#include <iostream>

#include <QSettings>
#include <QWidget>

#include "Monitor.h"
#include "ToggleWidgetOpenAction.h"

using namespace std;

MonitorView::MonitorView( Monitor *_monitor,
						  int _defaultWidth,
						  int _defaultHeight,
						  bool _fixedSize )
	: QGLWidget( NULL, NULL, Qt::Tool )
	, monitor( _monitor )
	, defaultWidth( _defaultWidth )
	, defaultHeight( _defaultHeight )
	, fixedSize( _fixedSize )
{
	setWindowTitle( monitor->getTitle() );
}

MonitorView::~MonitorView()
{
	saveSettings();
}

Monitor *MonitorView::getMonitor()
{
	return monitor;
}

void MonitorView::loadSettings()
{
	QSettings settings;
	settings.beginGroup( "monitorViews" );
	settings.beginGroup( monitor->getId() );

	if( settings.contains("x") && settings.contains("y") )
	{
		move( settings.value("x").toInt(),
			  settings.value("y").toInt() );
	}

	int w = fixedSize ? defaultWidth : settings.value( "w", defaultWidth ).toInt();
	int h = fixedSize ? defaultHeight : settings.value( "h", defaultHeight ).toInt();

	if( (w > 0) && (h > 0) )
	{
		if( fixedSize )
			setFixedSize( w, h );
		else
			resize( w, h );
	}

	if( settings.value("visible", false).toBool() )
	{
		show();
	}
}

void MonitorView::saveSettings()
{
	QSettings settings;
	settings.beginGroup( "monitorViews" );
	settings.beginGroup( monitor->getId() );
	
	settings.setValue( "visible", isVisible() );
	settings.setValue( "x", x() );
	settings.setValue( "y", y() );

	if( !fixedSize )
	{
		settings.setValue( "w", width() );
		settings.setValue( "h", height() );
	}
}
