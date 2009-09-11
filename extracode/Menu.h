#ifndef MENU_H
#define MENU_H


// qt
#include <qapplication.h>
#include <qmenubar.h>


class TMenu
{
    Q_OBJECT
public:
    TMenu(QApplication* app);

public slots:
    void open();
    void news();
    void save();
    void closeDoc();
    void undo();
    void redo();
    void normal();
    void bold();
    void underline();
    void about();
    void printer();
    void file();
    void fax();
    void printerSetup();

protected:

signals:
    void explain(const QString&);

private:
	QApplication* fApp;
    QMenuBar* menu;
    QLabel* label;
    bool isBold;
    bool isUnderline;
    int boldID;
    int underlineID;
};

#endif

