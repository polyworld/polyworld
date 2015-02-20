conf=../../../Makefile.conf
include ${conf}

target=${CALCCOMP_TARGET}
blddir=${CALCCOMP_BLDDIR}

cxxflags=${CXXFLAGS} ${GSL_CXXFLAGS} ${LIBRARY_CXXFLAGS} ${OMP_CXXFLAGS}
ldflags=${PWLIB_LDFLAGS}
libs=${GSL_LIBS} ${LIBRARY_LIBS} ${OMP_LIBS} ${QTRENDERER_LIBS} #todo: nullrenderer instead of qtrenderer

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
