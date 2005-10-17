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
				$(QT_INCLUDE_DIR) \
				$(QT_INCLUDE_DIR)/QtOpenGL \
				$(QT_INCLUDE_DIR)/QtGui \
				$(QT_INCLUDE_DIR)/QtCore \
				/System/Library/Frameworks/AGL.framework/Versions/A/Headers/
								
QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

LIBS	+=	-L$(QTDIR)/lib/ -framework QtOpenGL -framework OpenGL -framework AGL

