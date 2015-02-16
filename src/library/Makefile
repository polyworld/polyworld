conf=../../Makefile.conf
include ${conf}

target=${LIBRARY_TARGET}
blddir=${LIBRARY_BLDDIR}

cxxflags=${CXXFLAGS} ${OPENGL_CXXFLAGS} ${OMP_CXXFLAGS} ${CPPPROPS_CXXFLAGS}
ldflags=${SHARED_LDFLAGS}
libs=${OPENGL_LIBS} ${DLOPEN_LIBS} ${GSL_LIBS} ${OMP_LIBS}

includes=$(patsubst %, -I%, $(shell find . -type d))
sources=$(shell find . -name "*.cc")
objects=$(patsubst ./%.cc, ${blddir}/%.o, ${sources})
depends=$(patsubst %.o, %.d, ${objects})

${target}: ${objects}
	@mkdir -p $(shell dirname $@)
	${LD} ${ldflags} -o $@ ${objects} ${libs}

${blddir}/%.o: ./%.cc
	@mkdir -p $(shell dirname $@)
	${CXX} ${cxxflags} ${includes} -o $@ $<

-include ${depends}