TEMPLATE	= 	app

CONFIG		+=	qt warn_on debug

QT              +=	opengl

HEADERS		= 	app/PWApp.h	\
				app/Simulation.h

SOURCES		=	app/debug.cp \
				app/globals.cp \
				app/PWApp.cp \
				app/Simulation.cp \
				complexity/complexity_algorithm.cp \
				complexity/complexity_brain.cp \
				agent/brain.cp \
				agent/agent.cp \
				agent/genome.cp \
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
				motion/PositionPath.cp \
				motion/PositionWriter.cp \
				ui/BrainMonitorWindow.cp \
				ui/ChartWindow.cp \
				ui/AgentPOVWindow.cp \
				ui/SceneView.cp \
				ui/TextStatusWindow.cp \
				ui/OverheadView.cp \
				utils/error.cp \
				utils/indexlist.cp \
				utils/misc.cp \
				utils/PwMovieUtils.cp \
				utils/Resources.cp \
				utils/objectxsortedlist.cp \
				utils/distributions.cp

# Make source paths absolute so that XCode can properly interpret error messages
for(f, SOURCES): __SOURCES += $(DIR__HOME)/$${f}
SOURCES = $${__SOURCES}

INCLUDEPATH +=	. \
		agent \
		app \
		complexity \
		environment \
		graphics \
		motion \
		ui \
		utils \

macx {
	message(compiling for Mac OS X)
	INCLUDEPATH +=	/System/Library/Frameworks/AGL.framework/Versions/A/Headers/
}

unix:!macx {
	message(compiling for Linux)
	INCLUDEPATH +=	/usr/include/GL/
}

win32 {
}

# _GLIBCXX_DEBUG was added here, but it causes gdb problems on linux (bad in-memory datastructure layouts)
QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG_PEDANTIC -Wno-deprecated
QMAKE_CFLAGS += -Wno-deprecated

macx {
    # _GLIBCXX_DEBUG added here because it causes gdb problems on linux (bad in-memory datastructure layouts)
	QMAKE_CFLAGS_DEBUG += -D_GLIBCXX_DEBUG
	LIBS	+=	-framework AGL -lgsl -lgslcblas -lm
}

unix:!macx {
	LIBS	+=	-lgsl -lgslcblas

	# OpenMP Flags
	LIBS += -lgomp
	QMAKE_CFLAGS_DEBUG += -fopenmp
	QMAKE_CFLAGS += -fopenmp
}

win32 {
	message(Not building on Windows yet)
}

!macx:!unix:!win32 {
	message(Unknown platform - NOT Mac OS X or Windows or Unix)
}
