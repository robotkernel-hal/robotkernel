BUILT_SOURCES = ln_kernel_messages.h
CLEANFILES = ln_kernel_messages.h
includedir = $(prefix)/include/robotkernel

bin_PROGRAMS = robotkernel
include_HEADERS = module_intf.h module.h interface.h exceptions.h kernel.h kernel_worker.h runnable.h trigger_base.h \
				  log_thread.h dump_log.h ln_kernel_messages.h interface_intf.h helpers.h

robotkernel_SOURCES = module.cpp module_intf.cpp interface.cpp main.cpp exceptions.cpp kernel.cpp kernel_worker.cpp \
					  runnable.cpp trigger_base.cpp log_thread.cpp dump_log.cpp char_ringbuffer.cpp

robotkernel_CXXFLAGS = -I$(top_builddir) -I$(top_srcdir) @YAMLCPP_CFLAGS@ @LN_CFLAGS@
robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic @ROBOTKERNEL_LDFLAGS@
robotkernel_LDADD = @DL_LIBS@ @SOCKET_LIBS@ @RT_LIBS@ @PTHREAD_LIBS@ @YAMLCPP_LIBS@ @LN_LIBS@ @STRING_UTILS_LIBS@ #-lstdc++

if HAVE_LTTNG_UST
robotkernel_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_LDADD += @LTTNG_UST_LIBS@
robotkernel_SOURCES += dump_log_tp.c
BUILT_SOURCES += dump_log_tp.h dump_log_tp.c
endif

VPATH = $(srcdir):$(srcdir)/message_definitions 

LN_KERNEL_MESSAGES = robotkernel/add_module \
			  robotkernel/remove_module \
			  robotkernel/reconfigure_module \
			  robotkernel/set_state \
			  robotkernel/get_state \
			  robotkernel/get_states \
			  robotkernel/module_list \
			  robotkernel/get_dump_log \
			  robotkernel/config_dump_log \
			  robotkernel/module/get_feat \
			  robotkernel/module/get_config 

dump_log_tp.c dump_log_tp.h: dump_log_tp.tp
	@echo creating $@ from $^
	/usr/local/bin/lttng-gen-tp -o $@ $^

include $(top_srcdir)/Makefile.defs


