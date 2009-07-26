DIR__MOTION = ${DIR__HOME}/motion
DIR__COMPLEXITY = ${DIR__HOME}/complexity

INCLUDES = -I${DIR__COMPLEXITY} -I${DIR__MOTION}

MOTION_HEADERS = $(foreach MODULE,Path Reader Record,${DIR__MOTION}/Position${MODULE}.h)
MOTION_SOURCES = $(foreach MODULE,Path Reader,${DIR__MOTION}/Position${MODULE}.cp)

COMPLEXITY_HEADERS = ${wildcard ${DIR__COMPLEXITY}/*.h}
COMPLEXITY_SOURCES = ${wildcard ${DIR__COMPLEXITY}/*.cp}

HEADERS = ${wildcard *.h} ${MOTION_HEADERS} ${COMPLEXITY_HEADERS} 
SOURCES = ${wildcard *.cpp} ${MOTION_SOURCES} ${COMPLEXITY_SOURCES}

# make full paths so that XCode can parse any error messages
HEADERS := ${foreach file,${HEADERS}, ${shell readlink -f ${file}}}
SOURCES := ${foreach file,${SOURCES}, ${shell readlink -f ${file}}}

LIBS = -lgsl -lgslcblas -lm

CXX = g++

#CXX_FLAGS = -O3
CXX_FLAGS = -g
CXX_FLAGS += ${LIBS} ${INCLUDES}

ifeq (${OS}, Linux)
# respect OpenMP pragmas
CXX_FLAGS += -fopenmp
LIBS += -lgomp
endif

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
