
// Self
#include "Menu.h"

// qt
#include <qcursor.h>
#include <qpopupmenu.h>
#include <qapplication.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qpainter.h>


TMenu::TMenu()
	: fApp(app)
{
    QPixmap p1( p1_xpm );
    QPixmap p2( p2_xpm );
    QPixmap p3( p3_xpm );
    QPopupMenu *print = new QPopupMenu( fApp );
    Q_CHECK_PTR( print );
    print->insertTearOffHandle();
    print->insertItem( "&Print to printer", fApp, SLOT(printer()) );
    print->insertItem( "Print to &file", fApp, SLOT(file()) );
    print->insertItem( "Print to fa&x", fApp, SLOT(fax()) );
    print->insertSeparator();
    print->insertItem( "Printer &Setup", fApp, SLOT(printerSetup()) );

    QPopupMenu *file = new QPopupMenu( fApp );
    Q_CHECK_PTR( file );
    file->insertItem( p1, "&Open",  fApp, SLOT(open()), CTRL+Key_O );
    file->insertItem( p2, "&New", fApp, SLOT(news()), CTRL+Key_N );
    file->insertItem( p3, "&Save", fApp, SLOT(save()), CTRL+Key_S );
    file->insertItem( "&Close", fApp, SLOT(closeDoc()), CTRL+Key_W );
    file->insertSeparator();
    file->insertItem( "&Print", print, CTRL+Key_P );
    file->insertSeparator();
    file->insertItem( "E&xit",  qApp, SLOT(quit()), CTRL+Key_Q );

    QPopupMenu *edit = new QPopupMenu( fApp );
    Q_CHECK_PTR( edit );
    int undoID = edit->insertItem( "&Undo", fApp, SLOT(undo()) );
    int redoID = edit->insertItem( "&Redo", fApp, SLOT(redo()) );
    edit->setItemEnabled( undoID, FALSE );
    edit->setItemEnabled( redoID, FALSE );

    QPopupMenu* options = new QPopupMenu( fApp );
    Q_CHECK_PTR( options );
    options->insertTearOffHandle();
    options->setCaption("Options");
    options->insertItem( "&Normal Font", fApp, SLOT(normal()) );
    options->insertSeparator();


    QPopupMenu *help = new QPopupMenu( fApp );
    Q_CHECK_PTR( help );
    help->insertItem( "&About", fApp, SLOT(about()), CTRL+Key_H );

    // If we used a QMainWindow we could use its built-in menuBar().
    menu = new QMenuBar();
    Q_CHECK_PTR( menu );
    menu->insertItem( "&File", file );
    menu->insertItem( "&Edit", edit );
    menu->insertItem( "&Options", options );
    menu->insertSeparator();
    menu->insertItem( "&Help", help );
    menu->setSeparator( QMenuBar::InWindowsStyle );

#if 0

    QLabel *msg = new QLabel( fApp );
    Q_CHECK_PTR( msg );
    msg->setText( "A context menu is available.\n"
		  "Invoke it by right-clicking or by"
		  " pressing the 'context' button." );
    msg->setGeometry( 0, height() - 60, width(), 60 );
    msg->setAlignment( AlignCenter );

    label = new QLabel( fApp );
    Q_CHECK_PTR( label );
    label->setGeometry( 20, rect().center().y()-20, width()-40, 40 );
    label->setFrameStyle( QFrame::Box | QFrame::Raised );
    label->setLineWidth( 1 );
    label->setAlignment( AlignCenter );

    connect( fApp,  SIGNAL(explain(const QString&)),
	     label, SLOT(setText(const QString&)) );

    setMinimumSize( 100, 80 );
    setFocusPolicy( QWidget::ClickFocus );
#endif
}

void TMenu::open()
{
    emit explain( "File/Open selected" );
}


void TMenu::news()
{
    emit explain( "File/New selected" );
}

void TMenu::save()
{
    emit explain( "File/Save selected" );
}


void TMenu::closeDoc()
{
    emit explain( "File/Close selected" );
}


void TMenu::undo()
{
    emit explain( "Edit/Undo selected" );
}


void TMenu::redo()
{
    emit explain( "Edit/Redo selected" );
}


void TMenu::normal()
{
    isBold = FALSE;
    isUnderline = FALSE;
    QFont font;
    label->setFont( font );
    menu->setItemChecked( boldID, isBold );
    menu->setItemChecked( underlineID, isUnderline );
    emit explain( "Options/Normal selected" );
}


void TMenu::bold()
{
    isBold = !isBold;
    QFont font;
    font.setBold( isBold );
    font.setUnderline( isUnderline );
    label->setFont( font );
    menu->setItemChecked( boldID, isBold );
    emit explain( "Options/Bold selected" );
}


void TMenu::underline()
{
    isUnderline = !isUnderline;
    QFont font;
    font.setBold( isBold );
    font.setUnderline( isUnderline );
    label->setFont( font );
    menu->setItemChecked( underlineID, isUnderline );
    emit explain( "Options/Underline selected" );
}


void TMenu::about()
{
    QMessageBox::about( fApp, "PolyWorld", "AI Simulator");
}


void TMenu::printer()
{
    emit explain( "File/Printer/Print selected" );
}

void TMenu::file()
{
    emit explain( "File/Printer/Print To File selected" );
}

void TMenu::fax()
{
    emit explain( "File/Printer/Print To Fax selected" );
}

void TMenu::printerSetup()
{
    emit explain( "File/Printer/Printer Setup selected" );
}

