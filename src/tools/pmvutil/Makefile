conf=../../../Makefile.conf
include ${conf}

target=${PMVUTIL_TARGET}
blddir=${PMVUTIL_BLDDIR}

cxxflags=${CXXFLAGS} ${GSL_CXXFLAGS} ${LIBRARY_CXXFLAGS}
libs=${GSL_LIBS} ${LIBRARY_LIBS} ${QTRENDERER_LIBS} #todo: nullrenderer instead of qtrenderer

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
