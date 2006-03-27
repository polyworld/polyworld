TEMPLATE	= 	app

CONFIG		+=	qt warn_on debug

SUBDIRS  	=	PwMoviePlayer

HEADERS		= 	app/PWApp.h	\
				app/Simulation.h

SOURCES		=	app/debug.cp \
				app/globals.cp \
				app/PWApp.cp \
				app/Simulation.cp \
				critter/brain.cp \
				critter/critter.cp \
				critter/genome.cp \
				environment/barrier.cp \
				environment/food.cp \
				graphics/gcamera.cp \
				graphics/glight.cp \
				graphics/gline.cp \
				graphics/gmisc.cp \
				graphics/gobject.cp \
				graphics/gpolygon.cp \
				graphics/grect.cp \
				graphics/gscene.cp \
				graphics/gsquare.cp \
				graphics/gstage.cp \
				ui/BrainMonitorWindow.cp \
				ui/ChartWindow.cp \
				ui/CritterPOVWindow.cp \
				ui/SceneView.cp \
				ui/TextStatusWindow.cp \
				ui/OverheadView.cp \ 
				utils/error.cp \
				utils/indexlist.cp \
				utils/misc.cp \
				utils/PwMovietools.cp
																			  	
TARGET		= 	Polyworld

macx {
	INCLUDEPATH +=	. \
					app \
					critter \
					environment \
					graphics \
					ui \
					utils \
					$(QT_INCLUDE_DIR) \
					$(QT_INCLUDE_DIR)/QtOpenGL \
					/System/Library/Frameworks/AGL.framework/Versions/A/Headers/
}

unix:!macx {
	INCLUDEPATH +=	. \
					app \
					critter \
					environment \
					graphics \
					ui \
					utils \
					$(QT_INCLUDE_DIR) \
					$(QT_INCLUDE_DIR)/QtOpenGL \
					$(QT_INCLUDE_DIR)/QtCore \
					$(QT_INCLUDE_DIR)/QtGui \
					/usr/include/GL/
}

win32 {
	INCLUDEPATH +=	. \
					app \
					critter \
					environment \
					graphics \
					ui \
					utils \
					$(QT_INCLUDE_DIR) \
					$(QT_INCLUDE_DIR)/QtOpenGL				
}
								
QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -Wno-deprecated
QMAKE_CFLAGS += -Wno-deprecated

macx {
	LIBS	+=	-F$(QTDIR)/lib/ -framework QtOpenGL -framework OpenGL -framework AGL -lgsl -lgslcblas -lm
}

unix:!macx {
	LIBS	+=	-L$(QTDIR)/lib/ -lQtOpenGL -lgsl -lgslcblas
}

win32 {
	message(Not building on Windows yet)
}

!macx:!unix:!win32 {
	message(Unknown platform - NOT Mac OS X or Windows or Unix)
}
