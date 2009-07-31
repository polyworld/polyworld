INCLUDES = -I${DIR__MOTION}

MOTION_HEADERS = $(foreach MODULE,Path Reader Record,${DIR__MOTION}/Position${MODULE}.h)
MOTION_SOURCES = $(foreach MODULE,Path Reader,${DIR__MOTION}/Position${MODULE}.cp)

HEADERS = ${wildcard *.h} ${MOTION_HEADERS}
SOURCES = ${wildcard *.cpp} ${MOTION_SOURCES}

# make full paths so that XCode can parse any error messages
HEADERS := ${foreach file,${HEADERS}, ${shell readlink -f ${file}}}
SOURCES := ${foreach file,${SOURCES}, ${shell readlink -f ${file}}}

CXX = g++

#CXX_FLAGS = -O3
CXX_FLAGS = -g
CXX_FLAGS += ${INCLUDES}

ifeq (${OS}, Darwin)
CXX_FLAGS += -I/sw/include -L/sw/lib
endif


.PHONY: default clean

default: ${TARGET}

clean:
	rm -f ${TARGET}

${TARGET}: ${SOURCES} ${HEADERS}
	@echo ---------------------------------
	@echo --- BUILDING: $@
	@echo ---------------------------------

	${CXX} ${CXX_FLAGS} ${SOURCES} -o $@

	@echo ---------------------------------
	@echo --- SUCCESS: $@
	@echo ---------------------------------
