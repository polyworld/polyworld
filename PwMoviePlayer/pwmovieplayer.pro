TEMPLATE	= 	app

CONFIG		+=	qt warn_on debug

HEADERS		= 	PwMoviePlayer.h \
				MainWindow.h \
				GLWidget.h \
				../utils/PwMovieTools.h

SOURCES		=	PwMoviePlayer.cp \
				MainWindow.cp \
				GLWidget.cp \
				../utils/PwMovieTools.cp
																			  	
TARGET		= 	PwMoviePlayer

INCLUDEPATH +=	. \
				.. \
				../utils \
				$(QT)/include \
				$(QT)/include/QtOpenGL \
				$(QT)/include/QtGui \
				$(QT)/include/QtCore \
				/System/Library/Frameworks/AGL.framework/Versions/A/Headers
								
QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

LIBS	+=	-L$(QT)/lib -F$(QT)/lib -framework QtOpenGL -framework OpenGL -framework AGL

