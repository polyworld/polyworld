conf=../../Makefile.conf
include ${conf}

target=${LIBRARY_TARGET}
blddir=${LIBRARY_BLDDIR}

cxxflags=-I./ ${CXXFLAGS} ${OPENGL_CXXFLAGS} ${OMP_CXXFLAGS} ${CPPPROPS_CXXFLAGS}
ldflags=${SHARED_LDFLAGS}
libs=${OPENGL_LIBS} ${DLOPEN_LIBS} ${GSL_LIBS} ${OMP_LIBS} ${ZIP_LIBS}

include ${TARGET_MAK}