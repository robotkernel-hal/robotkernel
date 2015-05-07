
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
            if test $host = i686-suse-linux-gnu; then
               rmpm_host=sled11-x86-gcc4.x
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
        fi
        AC_SUBST(rmpm_host)
        ])

AC_DEFUN([RMPM_CHECK_MODULES], 
         [
         AC_REQUIRE([RMPM_ARCH])
         AC_MSG_CHECKING(for $1 -> searching package $2 in rmpm)
         tmp=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --search "$2" | wc -l)
         if test $tmp = 0; then
             AC_MSG_RESULT(no)
         else
             AC_MSG_RESULT(yes)
             
             $1[]_CFLAGS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/ctool --allow-beta --noquotes --c++ --compiler-flags --arch=$rmpm_host "$2")
             $1[]_LIBS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/ctool --allow-beta --noquotes --c++ --linker-flags --arch=$rmpm_host "$2")
             $1[]_BASE=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --allow-beta --key=PKGROOT "$2")
             $1[]_DEPENDS=$(env -i /volume/software/common/packages/rmpm/latest/bin/sled11-x86-gcc4.x/pkgtool --allow-beta --key=DEPENDS "$2")

             AC_SUBST($1[]_CFLAGS)
             AC_SUBST($1[]_LIBS)
             AC_SUBST($1[]_BASE)
             AC_SUBST($1[]_DEPENDS)
         fi])
         
