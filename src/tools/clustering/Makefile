conf=../../../Makefile.conf
include ${conf}

target=${QTCLUST_TARGET}
blddir=${QTCLUST_BLDDIR}

cxxflags=${CXXFLAGS} ${GSL_CXXFLAGS} ${OMP_CXXFLAGS} ${ZIP_CXXFLAGS}
libs=${GSL_LIBS} ${OMP_LIBS} ${ZIP_LIBS}

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
