conf=../../../Makefile.conf
include ${conf}

target=${OMPTEST_TARGET}
blddir=${OMPTEST_BLDDIR}

cxxflags=${CXXFLAGS} ${OMP_CXXFLAGS}
libs=${OMP_LIBS}

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
