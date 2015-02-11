PWHOME=~/polyworld/git/sean/polyworld
PWLIB=${PWHOME}/lib
PWBLD=${PWHOME}/bld
CXX=g++
LD=g++

OPENGL_CXXFLAGS=-I/usr/include/GL
OPENGL_LDFLAGS=-lGL -lGLU
PYTHON_CXXFLAGS=-I/usr/include/python2.7

cxxflags=-g -std=c++11 -c ${OPENGL_CXXFLAGS} ${PYTHON_CXXFLAGS}
ldflags=-fPIC -shared ${OPENGL_LDFLAGS}

blddir=${PWBLD}/core

includes=$(patsubst %, -I%, $(shell find . -type d))
sources=$(shell find . -name "*.cc")
objects=$(patsubst ./%.cc, ${blddir}/%.o, ${sources})

target=${PWLIB}/libpolyworld.so

${target}: ${objects}
	@mkdir -p $(shell dirname $@)
	${LD} -o $@ ${objects} ${ldflags}

${blddir}/%.o: ./%.cc
	@mkdir -p $(shell dirname $@)
	${CXX} ${cxxflags} ${includes} -o $@ $<