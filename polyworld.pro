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
				environment/brick.cp \
				environment/FoodPatch.cp \
				environment/BrickPatch.cp \
				environment/Patch.cp \
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
				utils/PwMovieTools.cp \
				utils/objectxsortedlist.cp \
				utils/distributions.cp
																			  	
TARGET		= 	Polyworld

macx {
	message(compiling for Mac OS X)
	INCLUDEPATH +=	. \
					app \
					critter \
					environment \
					graphics \
					ui \
					utils \
					$(QT)/include \
					$(QT)/include/QtOpenGL \
					/sw/include \
					/System/Library/Frameworks/AGL.framework/Versions/A/Headers/
}

unix:!macx {
	message(compiling for Linux)
	INCLUDEPATH +=	. \
					app \
					critter \
					environment \
					graphics \
					ui \
					utils \
					$(QT)/include \
					$(QT)/include/QtOpenGL \
					$(QT)/include/QtCore \
					$(QT)/include/QtGui \
					/usr/include/GL/
}

win32 {
	message(compiling for Windows)
	INCLUDEPATH +=	. \
					app \
					critter \
					environment \
					graphics \
					ui \
					utils \
					$(QT)/include \
					$(QT)/include/QtOpenGL				
}
								
QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -Wno-deprecated
QMAKE_CFLAGS += -Wno-deprecated

macx {
	LIBS	+=	-F$(QT)/lib/ -L/sw/lib -framework QtOpenGL -framework OpenGL -framework AGL -lgsl -lgslcblas -lm
}

unix:!macx {
	LIBS	+=	-L$(QT)/lib/ -lQtOpenGL -lgsl -lgslcblas
}

win32 {
	message(Not building on Windows yet)
}

!macx:!unix:!win32 {
	message(Unknown platform - NOT Mac OS X or Windows or Unix)
}
