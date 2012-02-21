#include "ToggleWidgetOpenAction.h"

#include <iostream>

#include <QEvent>

ToggleWidgetOpenAction::ToggleWidgetOpenAction( QObject *_parent,
													  QWidget *_widget,
													  QString _title )
	: QAction( _parent )
	, widget( _widget )
{
	setText( _title );
	setCheckable( true );
	setChecked( widget->isVisible() );

	widget->installEventFilter( this );
	connect( this, SIGNAL(toggled(bool)),
			 this, SLOT(onToggled(bool)) );
}

ToggleWidgetOpenAction::~ToggleWidgetOpenAction()
{
}

bool ToggleWidgetOpenAction::eventFilter( QObject *obj, QEvent *event )
{
	switch( event->type() )
	{
	case QEvent::Show:
		setChecked( true );
		break;
	case QEvent::Close:
		setChecked( false );
		break;
	default:
		break;
	}

	return false;
}

void ToggleWidgetOpenAction::onToggled( bool checked )
{
	if( checked != widget->isVisible() )
	{
		if( checked )
			widget->show();
		else
			widget->close();
	}
}
