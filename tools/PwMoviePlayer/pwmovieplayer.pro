TEMPLATE	= 	app

CONFIG		+=	qt warn_on debug

QT			+=	opengl

HEADERS		= 	PwMoviePlayer.h \
				MainWindow.h \
				GLWidget.h \
				$(DIR__UTILS)/PwMovieUtils.h

SOURCES		=	PwMoviePlayer.cp \
				MainWindow.cp \
				GLWidget.cp \
				$(DIR__UTILS)/PwMovieUtils.cp																			  	

INCLUDEPATH += $(DIR__UTILS)

macx {
	message(compiling for Mac OS X)
	INCLUDEPATH +=	/System/Library/Frameworks/AGL.framework/Versions/A/Headers
}

unix:!macx {
	message(compiling for Linux)
	INCLUDEPATH +=	/usr/include/GL/
}

QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -Wno-deprecated
QMAKE_CFLAGS += -Wno-deprecated

win32 {
	message(Not building on Windows yet)
}

!macx:!unix:!win32 {
	message(Unknown platform - NOT Mac OS X or Windows or Unix)
}
