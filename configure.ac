#												-*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.63])

#
# Robotkernel Version and Maintainer are extracted from project.properties file, change
# them there if needed
#
AC_INIT([robotkernel],
	m4_esyscmd_s([cat project.properties | grep VERSION    | cut -d'=' -f2 | xargs printf "%s"]),
	m4_esyscmd_s([cat project.properties | grep MAINTAINER | cut -d'=' -f2 | xargs printf "%s"]))

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/main/cpp/include/robotkernel/config.h])
#AM_INIT_AUTOMAKE([-Wall -Werror foreign silent-rules])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

DX_INIT_DOXYGEN([robotkernel-5], [doxyfile], [doc])
DX_HTML_FEATURE(ON)

#
# Checks for programs.
#
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CC
AX_CHECK_COMPILE_FLAG([-std=gnu11], [CFLAGS+=" -std=gnu11"], [AC_PROG_CC_C99])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11([ext], [mandatory])
AC_PROG_LIBTOOL
AC_PROG_INSTALL
m4_ifdef([PKG_PROG_PKG_CONFIG], [PKG_PROG_PKG_CONFIG])
m4_ifdef([PKG_INSTALLDIR], [PKG_INSTALLDIR], [pkgconfigdir='${libdir}/pkgconfig' AC_SUBST([pkgconfigdir])])

PKG_CHECK_MODULES([LTTNG_UST], [lttng-ust], [enable_lttng=yes], [enable_lttng=no])
PKG_CHECK_MODULES([LIBSTRING_UTIL], [libstring_util])
PKG_CHECK_MODULES([YAML_CPP], [yaml-cpp], [], [AC_MSG_ERROR([yaml-cpp not found])])

AM_CONDITIONAL([HAVE_LTTNG_UST], [test "$enable_lttng" == "yes"])
AS_IF([test "$enable_lttng" = "yes"], [AC_DEFINE([HAVE_LTTNG_UST], [1], [Use it])], [AC_DEFINE([HAVE_LTTNG_UST], [0], [Dont use it])])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h sys/ioctl.h unistd.h execinfo.h termios.h sys/syscall.h process.h libgen.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_CHECK_PTHREAD_SETNAME_NP

# Checks for library functions.
#AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memset sqrt strerror gettid])

# Checks for library functions.
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(rt, clock_gettime)

AX_CHECK_LINK_FLAG(-non-static, [LDFLAGS+=" -non-static"])
AX_CHECK_LINK_FLAG(-lSegFault, [LDFLAGS+=" -lSegFault"])

AC_CHECK_TYPES([cpu_set_t], [], [], [[#include <pthread.h>]])

# pthread support
AC_CHECK_LIB(pthread, pthread_create, PTHREAD_LIBS="-lpthread")
AC_SUBST(PTHREAD_LIBS)

AC_CONFIG_FILES([Makefile src/main/cpp/Makefile robotkernel.pc])

AC_OUTPUT
