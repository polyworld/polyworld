TEMPLATE	= 	app

CONFIG		+=	qt warn_on debug

SUBDIRS  	=	ui

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
				utils/error.cp \
				utils/indexlist.cp \
				utils/misc.cp
																			  	
TARGET		= 	Polyworld

INCLUDEPATH +=	. \
				app \
				critter \
				environment \
				graphics \
				ui \
				utils
				
QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC		
											
REQUIRES	=	full-config
