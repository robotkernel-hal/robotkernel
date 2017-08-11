AC_DEFUN([AC_CHECK_PTHREAD_SETNAME_NP], [
    CFLAGS="-Wall -Werror=implicit-function-declaration"
    
    # pthread_setname is non-posix, and there are at least 4 different implementations
    AC_MSG_CHECKING([whether signature of pthread_setname_np() has 1 argument])
    AC_COMPILE_IFELSE(
    	[AC_LANG_PROGRAM(
    		[[
                #define _GNU_SOURCE
                #include <pthread.h>
            ]],
    		[[pthread_setname_np ("foo"); return 0;]])
    	],[
    		AC_MSG_RESULT([yes])
    		AC_DEFINE(HAVE_PTHREAD_SETNAME_NP_1, [1],
    		    [Whether pthread_setname_np() has 1 argument])
    	],[
    		AC_MSG_RESULT([no])
    ])
    AC_MSG_CHECKING([whether signature of pthread_setname_np() has 2 arguments])
    AC_COMPILE_IFELSE(
    	[AC_LANG_PROGRAM(
    		[[
                #define _GNU_SOURCE
                #include <pthread.h>
            ]],
    		[[pthread_setname_np (pthread_self (), "foo"); return 0;]])
    	],[
    		AC_MSG_RESULT([yes])
    		AC_DEFINE(HAVE_PTHREAD_SETNAME_NP_2, [1],
    		    [Whether pthread_setname_np() has 2 arguments])
    	],[
    		AC_MSG_RESULT([no])
    ])
    AC_MSG_CHECKING([whether signature of pthread_setname_np() has 3 arguments])
    AC_COMPILE_IFELSE(
    	[AC_LANG_PROGRAM(
    		[[
                #define _GNU_SOURCE
                #include <pthread.h>
            ]],
    		[[pthread_setname_np (pthread_self(), "foo", (void *)0); return 0;]])
    	],[
    		AC_MSG_RESULT([yes])
    		AC_DEFINE(HAVE_PTHREAD_SETNAME_NP_3, [1],
    		    [Whether pthread_setname_np() has 3 arguments])
    	],[
    		AC_MSG_RESULT([no])
    ])
    AC_MSG_CHECKING([whether pthread_set_name_np() exists])
    AC_COMPILE_IFELSE(
    	[AC_LANG_PROGRAM(
    		[[
                #define _GNU_SOURCE
                #include <pthread.h>
            ]],
    		[[pthread_set_name_np (pthread_self(), "foo"); return 0;]])
    	],[
    		AC_MSG_RESULT([yes])
    		AC_DEFINE(HAVE_PTHREAD_SET_NAME_NP, [1],
    		    [Whether pthread_set_name_np() exists])
    	],[
    		AC_MSG_RESULT([no])
    ])
])
