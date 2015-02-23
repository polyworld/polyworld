conf=../../Makefile.conf
include ${conf}

target=${LIBRARY_TARGET}
blddir=${LIBRARY_BLDDIR}

cxxflags=${CXXFLAGS} ${OPENGL_CXXFLAGS} ${OMP_CXXFLAGS} ${CPPPROPS_CXXFLAGS}
ldflags=${SHARED_LDFLAGS}
libs=${OPENGL_LIBS} ${DLOPEN_LIBS} ${GSL_LIBS} ${OMP_LIBS} ${ZIP_LIBS}

includes=$(patsubst %, -I%, $(shell find . -type d))
cxxflags += ${includes}

include ${TARGET_MAK}