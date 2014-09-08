#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([robotkernel], [3.0.9], [robert.burger@dlr.de])
AC_CANONICAL_TARGET
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#AM_PROG_AR
AC_CONFIG_HEADERS([include/robotkernel/config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_MAKEFLAGS="$AM_MAKEFLAGS --no-print-directory --jobs=8"

# load env-file with each make invocation
if test -n "$SOURCE_ENV"; then
   echo "loading $SOURCE_ENV"
   source $SOURCE_ENV
fi

# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_CC

if test -z "$MAKE"; then
   MAKE=make
fi
# expand path with each make invocation
if test -n "$TOOLCHAIN"; then
   MAKE="$MAKE PATH=$TOOLCHAIN:$PATH"
   PATH=$TOOLCHAIN:$PATH
fi
# load env-file with each make invocation
if test -n "$SOURCE_ENV"; then
   echo "loading $SOURCE_ENV"
   source $SOURCE_ENV
   MAKE="source $SOURCE_ENV; $MAKE"
fi
if test -n "$MAKE"; then
   SET_MAKE="MAKE := $MAKE"
fi
AC_SUBST(SET_MAKE)

# setup rmpm
RMPM_ARCH

# serach packages, first with pkg-config, second in rmpm  
# yaml-cpp 
PKG_CHECK_MODULES([YAMLCPP], [yaml-cpp], [], [RMPM_CHECK_MODULES([YAMLCPP], [software.yaml-cpp])])
AM_CONDITIONAL([HAVE_YAMLCPP], [test -n "$YAMLCPP_LIBS"])

if test "$rmpm_host" == "angstrom-armv7-gcc4.7"; then
   ARCH_CFLAGS="-DNO_RDTSC"
fi
AC_SUBST(ARCH_CFLAGS)

# try to automatically find ln
LN_GENERATE_ARGS='-md_dir $(srcdir)/message_definitions'
if test "${LN_LIBS}" == ""; then
    if test -n "${LN_BASE}"; then
        AC_CHECK_HEADER(${LN_BASE}/include/ln.h, [ 
                # found it in release dir!
                LN_LIBS="-L${LN_BASE}/lib/$rmpm_host -lln"
                LN_CFLAGS="-I${LN_BASE}/include/"
                LN_GENERATE="env -i ${LN_BASE}/scripts/ln_generate $LN_GENERATE_ARGS"
                LN_YAMLCPP_LIBS="-L${LN_BASE}/lib/$rmpm_host -lyaml-cpp"
                STRING_UTILS_LIBS="-L$LN_BASE/lib/$rmpm_host -lstring_util"
            ], [
                # not found...
                AC_CHECK_HEADER(${LN_BASE}/library/include/ln.h, [
                        # found it in source dir!
                        LN_LIBS="-L${LN_BASE}/library/obj/$rmpm_host -lln"
                        LN_CFLAGS="-I${LN_BASE}/library/include/ -I${LN_BASE}/contrib/libstring_util/include"
                        LN_GENERATE="env -i ${LN_BASE}/library/tools/ln_generate $LN_GENERATE_ARGS"
                        LN_YAMLCPP_CFLAGS="-I${LN_BASE}/contrib/libyaml/include"
                        LN_YAMLCPP_LIBS="-L${LN_BASE}/contrib/libyaml/lib/$rmpm_host -lyaml-cpp"
                        STRING_UTILS_LIBS="-L$LN_BASE/contrib/libstring_util/obj/$rmpm_host -lstring_util"
                    ])
            ])
    fi
fi

# try to find ln via pkg-config and rmpm, if not already done in above step
PKG_CHECK_MODULES([LN], [libln], [], [
                    RMPM_CHECK_MODULES([LN], [software.links_and_nodes])
                    LN_GENERATE="env -i $LN_BASE/scripts/ln_generate $LN_GENERATE_ARGS"
                    STRING_UTILS_LIBS="-L$LN_BASE/lib/$rmpm_host -lstring_util"
                   ])
AM_CONDITIONAL([HAVE_LN], [test -n "$LN_LIBS"])
AC_SUBST(LN_GENERATE)
AC_DEFINE_UNQUOTED(LN_LIBS, "$LN_LIBS", [linked links and nodes library])
AC_SUBST(LN_LIBS)
AC_SUBST(LN_VERSION)
AC_SUBST(YAML_VERSION)
AC_SUBST(STRING_UTILS_LIBS)

PKG_CHECK_MODULES([LTTNG_UST], [lttng-ust], [AC_DEFINE([HAVE_LTTNG_UST], [1], [use lttng user space tracing])], [AC_DEFINE([HAVE_LTTNG_UST], [0], [use lttng user space tracing])])
AM_CONDITIONAL([HAVE_LTTNG_UST], [test -n "$LTTNG_UST_LIBS"])

AM_CONDITIONAL([HAVE_PRU], [test -n "$PRU_BASE"])

AC_DEFINE_UNQUOTED(BUILD_DATE,"`date -u`",[Date (usually date of extraction from SVN).])
BUILD_DATE="`date -u`"
AC_SUBST(BUILD_DATE)

AC_DEFINE_UNQUOTED(BUILD_USER,"`whoami`",[User (usually user logged in on shell).])
BUILD_USER="`whoami`"
AC_SUBST(BUILD_USER)

AC_DEFINE_UNQUOTED(BUILD_HOST,"`hostname`",[Hostname (build host).])
BUILD_HOST="`hostname`"
AC_SUBST(BUILD_HOST)

ROBOTKERNEL_CFLAGS="-I$srcdir "
ROBOTKERNEL_LIBS="-L$builddir "
ROBOTKERNEL_LDFLAGS=""

MODULE_LDFLAGS=""

# Platform specific setup
# Check for which host we are on and setup a few things
# specifically based on the host
case $target in
    *vxworks* )
        ROBOTKERNEL_LDFLAGS+="$LDFLAGS -non-static " 
        ;;
    *qnx6.5* )
        ROBOTKERNEL_LDFLAGS+="$LDFLAGS -lcpp -lm "  
        MODULE_LDFLAGS+="-lgcc -lm "
        ;;
    *qnx6.3* )
        ROBOTKERNEL_LDFLAGS+="$LDFLAGS -lm -lstdc++"
        ;;
    *)
        ROBOTKERNEL_LDFLAGS+="$LDFLAGS -rdynamic " 
        ;;
esac
AC_SUBST([ROBOTKERNEL_CFLAGS])
AC_SUBST([ROBOTKERNEL_LIBS])
AC_SUBST([ROBOTKERNEL_LDFLAGS])
AC_SUBST([MODULE_LDFLAGS])

AC_ARG_WITH([debug],
            [AS_HELP_STRING([--with-debug],
                            [enable support for debug build])],
                            [with_debug=yes],
                            [with_debug=no])
AS_IF([test "x$with_debug" != xno], [ 
       CFLAGS="-Wall -g3 -O0"
       CXXFLAGS="-Wall -g3 -O0" ])

CFLAGS="-Wall"
CXXFLAGS="-Wall"

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h sys/ioctl.h unistd.h execinfo.h termios.h])

AM_CONDITIONAL([HAVE_TERMIOS], [test "$ac_cv_header_termios_h" == "yes"])
AC_SUBST(HAVE_TERMIOS)

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

# Checks for library functions.
#AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memset sqrt strerror])

# Checks for library functions.
AC_CHECK_LIB(dl, dlopen, DL_LIBS="-ldl", DL_LIBS=)
AC_SUBST(DL_LIBS)
AC_CHECK_LIB(socket, socket, SOCKET_LIBS="-lsocket", SOCKET_LIBS=)
AC_SUBST(SOCKET_LIBS)

# construction realtime libraries
AC_CHECK_LIB(rt, clock_gettime, RT_LIBS="-lrt")
AC_SUBST(RT_LIBS)

AC_CHECK_TYPES([cpu_set_t], [], [], [[#include <pthread.h>]])

# pthread support 
AC_CHECK_LIB(pthread, pthread_create, PTHREAD_LIBS="-lpthread")
AC_SUBST(PTHREAD_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_FILES([robotkernel.pt], [cp robotkernel.pt ${ac_abs_top_srcdir}])

AC_OUTPUT

