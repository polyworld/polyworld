conf=../../../Makefile.conf
include ${conf}

target=${CALCCOMP_TARGET}
blddir=${CALCCOMP_BLDDIR}

cxxflags=${CXXFLAGS} ${GSL_CXXFLAGS} ${LIBRARY_CXXFLAGS} ${OMP_CXXFLAGS}
ldflags=${PWLIB_LDFLAGS}
libs=${GSL_LIBS} ${LIBRARY_LIBS} ${OMP_LIBS} ${QTRENDERER_LIBS} #todo: nullrenderer instead of qtrenderer

include ${TARGET_MAK}