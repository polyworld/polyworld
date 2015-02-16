ifndef conf
error:
	@echo "cppprops.mak requires definition of env var 'conf'">&2
	@exit 1
endif

include ${conf}

target=${CPPPROPS_TARGET}
blddir=${CPPPROPS_BLDDIR}

cxxflags=${CXXFLAGS} ${LIBRARY_CXXFLAGS} ${OPENGL_CXXFLAGS} -Wno-unused-variable
ldflags=${SHARED_LDFLAGS}
libs=${LIBRARY_LIBS}

sources=$(shell find . -name "*.cc")
objects=$(patsubst ./%.cc, ${blddir}/%.o, ${sources})

${target}: ${objects}
	@mkdir -p $(shell dirname $@)
	${LD} ${ldflags} -o $@ ${objects} ${libs}

${blddir}/%.o: ./%.cc
	@mkdir -p $(shell dirname $@)
	${CXX} ${cxxflags} ${includes} -o $@ $<

