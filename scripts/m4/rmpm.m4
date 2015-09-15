
AC_DEFUN([RMPM_ARCH], 
        [
        AC_REQUIRE([AC_CANONICAL_HOST])
        rmpm_host=$host
        if test "$cross_compiling" == "no"; then
            # try to guess from current host
            MACHINE=$(uname -m)

            if test "$MACHINE" == x86_64; then
                rmpm_host=sled11-x86_64-gcc4.x
            else
                rmpm_host=sled11-x86-gcc4.x
            fi
        else
            ARCH_CFLAGS=""
            if test $host = i686-suse-linux-gnu; then
                rmpm_host=sled11-x86-gcc4.x
            fi
            if test $host = x86-sled11-gnu4.x; then
                rmpm_host=sled11-x86-gcc4.x
            fi
            if test $host = x86_64-sled11-gnu4.x; then
                rmpm_host=sled11-x86_64-gcc4.x
            fi
            if test $host = x86-vxworks6.7-gnu4.x; then
                rmpm_host=vxworks6.7-x86-gcc4.x
            fi
            if test $host = x86-vxworks6.8-gnu4.x; then
                rmpm_host=vxworks6.8-x86-gcc4.x
            fi
            if test $host = x86-vxworks6.9-gnu4.x; then
                rmpm_host=vxworks6.9-x86-gcc4.x
            fi
            if test $host = x86-vxworks6.9.3-gnu4.x; then
                rmpm_host=vxworks6.9.3-x86-gcc4.x
            fi            
            if test $host = x86-vxworks6.9.4-gnu4.x; then
                rmpm_host=vxworks6.9.4-x86-gcc4.x
            fi            
            if test $host = x86-qnx6.3-gnu3.3; then
                rmpm_host=qnx6.3-x86-gcc3.3
            fi
            if test $host = x86-qnx6.5-gnu4.x; then
                rmpm_host=qnx6.5-x86-gcc4.x
            fi
            if test $host = x86-winxp-mingw32; then
                rmpm_host=winxp-x86-mingw32
                ARCH_CFLAGS="-D__WIN32__"
            fi
            if test $host = arm-angstrom-linux-gnueabi; then
                rmpm_host=angstrom-armv7-gcc4.7
                ARCH_CFLAGS="-DNO_RDTSC"
            fi
            if test $host = arm-linux-gnueabihf; then
                rmpm_host=ubuntu12.04-armhf-gcc4.x
                ARCH_CFLAGS="-DNO_RDTSC"
            fi
            if test $host = arm-unknown-linux-gnueabihf; then
                rmpm_host=ubuntu12.04-armhf-gcc4.x
                ARCH_CFLAGS="-DNO_RDTSC"
            fi
        fi
        AC_SUBST(rmpm_host)
        AC_SUBST(ARCH_CFLAGS)

        ])

AC_DEFUN([RMPM_CHECK_MODULES], 
         [
         AC_REQUIRE([RMPM_ARCH])

	 # split package name ($2) in domain and package part!
	 m4_define([RMPMDOMAIN], m4_substr($2, 0, index($2, .)))
	 m4_define([RMPMPKG], m4_substr($2, m4_eval(index($2, .) + 1)))

	 # is there a command line option named package-domain? -> if so, use that domain!
	 AC_ARG_WITH(RMPMPKG[-domain],
	  AS_HELP_STRING([--with-RMPMPKG-domain], [specify another rmpm domain than RMPMDOMAIN]),
	  [domain="${withval}"
	   AC_MSG_CHECKING(for $1 -> searching rmpm package RMPMPKG in user-specified domain $domain)
	  ],
	  [domain=RMPMDOMAIN
	   AC_MSG_CHECKING(for $1 -> searching rmpm package RMPMPKG in domain $domain)
          ]
	 )
	 pkg_spec=$domain.RMPMPKG

         tmp=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --search "$pkg_spec" --arch=$rmpm_host | wc -l)
         if test $tmp = 0; then
             AC_MSG_RESULT(no)
         else             
             $1[]_BASE=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --allow-beta --key=PKGROOT "$pkg_spec" | sed -e "s/, .*$//")
             AC_MSG_RESULT(yes ${$1[]_BASE})
             $1[]_CFLAGS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/ctool --allow-beta --noquotes --c++ --compiler-flags --arch=$rmpm_host "$pkg_spec")
             $1[]_LIBS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/ctool --allow-beta --noquotes --c++ --linker-flags --arch=$rmpm_host "$pkg_spec")
             $1[]_DEPENDS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --allow-beta --key=DEPENDS "$pkg_spec")
             $1[]_CXX_DEPENDS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --allow-beta --key=C++_DEPENDS "$pkg_spec")
             $1[]_C_DEPENDS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --allow-beta --key=C_DEPENDS "$pkg_spec")

             AC_SUBST($1[]_CFLAGS)
             AC_SUBST($1[]_LIBS)
             AC_SUBST($1[]_BASE)
             AC_SUBST($1[]_DEPENDS)
             AC_SUBST($1[]_CXX_DEPENDS)
             AC_SUBST($1[]_C_DEPENDS)
         fi])
         
AC_DEFUN([HAVE_BY_CFLAGS], [
         AC_MSG_CHECKING(have $1?)

	 AM_CONDITIONAL([HAVE_$1], [test -n "$$1[]_CFLAGS"])
	 AS_IF([test -n "$$1[]_CFLAGS"], [AC_DEFINE([HAVE_$1], [1], [Define if have $1])])
	 AC_SUBST(HAVE_$1)

	 AS_IF([test -n "$$1[]_CFLAGS"], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no)])

])
