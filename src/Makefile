targets=all library app qtrenderer rancheck PwMoviePlayer proputil pmvutil qt_clust

.PHONY: ${targets}

all: ${targets}

library:
	+ make -C library/

qtrenderer: library
	+ make -C qtrenderer/

app: library qtrenderer
	+ make -C app/

rancheck:
	+ make -C tools/rancheck

PwMoviePlayer: library qtrenderer
	+ make -C tools/PwMoviePlayer

proputil: library qtrenderer #todo: nullrenderer instead of qtrenderer
	+ make -C tools/proputil

pmvutil: library qtrenderer #todo: nullrenderer instead of qtrenderer
	+ make -C tools/pmvutil

qt_clust:
	+ make -C tools/clustering