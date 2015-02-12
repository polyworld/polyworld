.PHONY: all library app qtrenderer

all: library app qtrenderer

library:
	+ make -C library/

qtrenderer: library
	+ make -C qtrenderer/

app: library qtrenderer
	+ make -C app/