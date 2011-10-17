# This file should never be executable

SCONS = scons -f scripts/build/SConstruct

.PHONY: all clean \
	app Polyworld \
	comp CalcComplexity \
	mp PwMoviePlayer \
	rancheck \
	proputil \
	qt_clust

all:
	${SCONS}

clean:
	${SCONS} --clean
	rm -rf .bld
	rm -rf bin
	rm -rf src # this just has symbolic links
	rm -f .sconsign.dblite
	rm -f config.log

app Polyworld:
	${SCONS} Polyworld

comp CalcComplexity:
	${SCONS} bin/CalcComplexity

mp PwMoviePlayer:
	${SCONS} bin/PwMoviePlayer

rancheck:
	${SCONS} bin/rancheck

proputil:
	${SCONS} bin/proputil

qt_clust:
	${SCONS} bin/qt_clust