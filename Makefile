include Makefile.conf

targets=library app qtrenderer rancheck PwMoviePlayer proputil pmvutil qt_clust passive nullevo neurons expansion bifurcation timeseries

.PHONY: ${targets} clean

all: ${targets}

library:
	+ make -C src/library/

qtrenderer: library
	+ make -C src/qtrenderer/

app: library qtrenderer
	+ make -C src/app/

rancheck:
	+ make -C src/tools/rancheck

PwMoviePlayer: library qtrenderer
	+ make -C src/tools/PwMoviePlayer

proputil: library qtrenderer #todo: nullrenderer instead of qtrenderer
	+ make -C src/tools/proputil

pmvutil: library qtrenderer #todo: nullrenderer instead of qtrenderer
	+ make -C src/tools/pmvutil

qt_clust:
	+ make -C src/tools/clustering

omp_test:
	+ make -C src/tools/omp_test
	bin/omp_test

passive:
	+ make -C src/tools/passive

nullevo:
	+ make -C src/tools/nullevo

neurons:
	+ make -C src/tools/neurons

expansion:
	+ make -C src/tools/expansion

bifurcation:
	+ make -C src/tools/bifurcation

timeseries:
	+ make -C src/tools/timeseries

clean:
	rm -rf ${PWBLD}
	rm -rf ${PWLIB}
	rm -rf ${PWBIN}
	rm -f ${APP_TARGET}
