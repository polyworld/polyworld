ifndef target
    $(error $${target} not defined)
endif

ifndef blddir
    $(error $${blddir} not defined)
endif

sources=$(shell find . -name "*.cc")
objects=$(patsubst ./%.cc, ${blddir}/%.o, ${sources})
depends=$(patsubst %.o, %.d, ${objects})

${target}: ${objects}
	@mkdir -p $(shell dirname $@)
	${LD} ${ldflags} -o $@ ${objects} ${libs}

${blddir}/%.o: ./%.cc
	@mkdir -p $(shell dirname $@)
	${CXX} ${cxxflags} -o $@ $<

clean:
	rm -rf ${blddir}
	rm -rf ${target}

-include ${depends}
