conf=../../../Makefile.conf
include ${conf}

target=${PROPUTIL_TARGET}
blddir=${PROPUTIL_BLDDIR}

cxxflags=${CXXFLAGS} ${GSL_CXXFLAGS} ${LIBRARY_CXXFLAGS}
ldflags=${PWLIB_LDFLAGS}
libs=${GSL_LIBS} ${LIBRARY_LIBS} ${QTRENDERER_LIBS} #todo: nullrenderer instead of qtrenderer

include ${TARGET_MAK}
