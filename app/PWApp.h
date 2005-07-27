#ifndef PWAPP_H
#define PWAPP_H

// qt
#include <QApplication>

//===========================================================================
// TPWApp
//===========================================================================
class TPWApp : public QApplication
{
Q_OBJECT

public:
	TPWApp(int argc, char** argv);
};

#endif
