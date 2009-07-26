TEMPLATE	= 	app

CONFIG		+=	qt warn_on debug

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
	INCLUDEPATH +=	. \
					$(QT)/include \
					$(QT)/include/QtOpenGL \
					$(QT)/include/QtGui \
					$(QT)/include/QtCore \
					/System/Library/Frameworks/AGL.framework/Versions/A/Headers
}

unix:!macx {
	message(compiling for Linux)
	INCLUDEPATH +=	. \
					$(QT)/include \
					$(QT)/include/QtOpenGL \
					$(QT)/include/QtCore \
					$(QT)/include/QtGui \
					/usr/include/GL/
}

QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -Wno-deprecated
QMAKE_CFLAGS += -Wno-deprecated

macx {
	LIBS +=	-L$(QT)/lib -F$(QT)/lib -framework QtOpenGL -framework OpenGL -framework AGL
}

unix:!macx {
	LIBS += -L$(QT)/lib/ -lQtOpenGL -lgsl -lgslcblas
}

win32 {
	message(Not building on Windows yet)
}

!macx:!unix:!win32 {
	message(Unknown platform - NOT Mac OS X or Windows or Unix)
}
