#pragma once

#include <QAction>

class ToggleWidgetOpenAction : public QAction
{
	Q_OBJECT

public:
	ToggleWidgetOpenAction( QObject *_parent,
							   QWidget *_widget,
							   QString _title );

	virtual ~ToggleWidgetOpenAction();

	bool eventFilter( QObject *obj, QEvent *event );

 private slots:
	void onToggled( bool checked );

 private:
	QWidget *widget;
};
