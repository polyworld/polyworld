conf=../../Makefile.conf
include ${conf}

cxxflags=${CXXFLAGS} ${OPENGL_CXXFLAGS} ${PYTHON_CXXFLAGS} ${OMP_CXXFLAGS}
ldflags=${SHARED_LDFLAGS}
libs=${OPENGL_LIBS} ${DLOPEN_LIBS} ${GSL_LIBS} ${PYTHON_LIBS} ${OMP_LIBS}

blddir=${LIBRARY_BLDDIR}
includes=$(patsubst %, -I%, $(shell find . -type d))
sources=$(shell find . -name "*.cc")
objects=$(patsubst ./%.cc, ${blddir}/%.o, ${sources})

target=${LIBRARY_TARGET}

${target}: ${objects}
	@mkdir -p $(shell dirname $@)
	${LD} ${ldflags} -o $@ ${objects} ${libs}

${blddir}/%.o: ./%.cc
	@mkdir -p $(shell dirname $@)
	${CXX} ${cxxflags} ${includes} -o $@ $<