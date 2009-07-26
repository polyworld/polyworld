SHELL = /bin/bash

export OS = ${shell uname}

ifeq (${OS},Darwin)
	APP_EXT = .app
else
	APP_EXT =
endif

###
### Targets
###
POLYWORLD = Polyworld
MOVIE_PLAYER = PwMoviePlayer
CALC_COMPLEXITY = CalcComplexity

APP__POLYWORLD = ${POLYWORLD}${APP_EXT}
APP__MOVIE_PLYAER = ${MOVIE_PLAYER}${APP_EXT}

###
### Directory Structure
###
export DIR__HOME = ${PWD}
DIR__BLD = ${DIR__HOME}/.bld
DIR__BLD_APP = ${DIR__BLD}/app
DIR__BLD_MOVIE_PLAYER = ${DIR__BLD}/${MOVIE_PLAYER}
DIR__BLD_CALC_COMPLEXITY = ${DIR__BLD}/${CALC_COMPLEXITY}

DIR__BIN = ${DIR__HOME}/bin
export DIR__TOOLS = ${DIR__HOME}/tools
export DIR__UTILS = ${DIR__HOME}/utils
DIR__CALC_COMPLEXITY = ${DIR__TOOLS}/${CALC_COMPLEXITY}
DIR__MOVIE_PLAYER = ${DIR__TOOLS}/${MOVIE_PLAYER}

###
### Target-specific files
###
MAKEFILE__APP = ${DIR__BLD_APP}/Makefile
QPRO__APP = polyworld.pro

MAKEFILE__MOVIE_PLAYER = ${DIR__BLD_MOVIE_PLAYER}/Makefile
QPRO__MOVIE_PLAYER = ${DIR__MOVIE_PLAYER}/pwmovieplayer.pro

TARGET__CALC_COMPLEXITY = ${DIR__BLD_CALC_COMPLEXITY}/${CALC_COMPLEXITY}

###
### Utilities
###
QMAKE = qmake

#################################################################

.PHONY: default clean env app movie_player calc_complexity

default: env app movie_player calc_complexity
	@echo ---------------------------------
	@echo --- SUCCESS: Polyworld Project
	@echo ---------------------------------

###
### Clean/Remove Build Output
###
clean:
	rm -rf ${DIR__BLD}
	rm -rf ${DIR__BIN}
	rm -rf ${APP__POLYWORLD}

###
### Pre-build Setup
###
env:
	@echo ---------------------------------
	@echo --- BUILDING: Polyworld Project
	@echo ---------------------------------
	@echo "<${MAKEFILE__APP}: ${QPRO__APP}>"
	mkdir -p ${DIR__BLD_APP}
	mkdir -p ${DIR__BLD_MOVIE_PLAYER}
	mkdir -p ${DIR__BLD_CALC_COMPLEXITY}
	mkdir -p ${DIR__BIN}

###
### Polyworld Application
###
app: ${MAKEFILE__APP}
	cd ${dir $<} && ${MAKE} --file ${notdir $<}
	cp -R ${dir $<}/${APP__POLYWORLD} ${DIR__HOME}
	ln -fs  ${DIR__HOME}/${APP__POLYWORLD} ${DIR__BIN}/${APP__POLYWORLD}

# generate Makefile via qmake
${MAKEFILE__APP}: ${QPRO__APP}
	${QMAKE} ${QPRO__APP} -o $@ "TARGET=${POLYWORLD}"

###
### Movie Player
###
movie_player: ${MAKEFILE__MOVIE_PLAYER}
	cd ${dir $<} && ${MAKE}
	cp -R ${dir $<}/${APP__MOVIE_PLAYER} ${DIR__BIN}

${MAKEFILE__MOVIE_PLAYER}: ${QPRO__MOVIE_PLAYER}
	${QMAKE} ${QPRO__MOVIE_PLAYER} -o $@ "TARGET=${MOVIE_PLAYER}"

###
### CalcComplexity
###
calc_complexity:
	cd ${DIR__CALC_COMPLEXITY} && ${MAKE} TARGET=${TARGET__CALC_COMPLEXITY}
	ln -sf ${TARGET__CALC_COMPLEXITY} ${DIR__BIN}