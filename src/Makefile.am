INSTALL_HEADER = $(INSTALL_DATA) -C

BUILT_SOURCES = $(top_builddir)/include/robotkernel/ln_kernel_messages.h
CLEANFILES = $(top_builddir)/include/robotkernel/ln_kernel_messages.h
includedir = $(prefix)/include/robotkernel
headerdir = $(top_srcdir)/include/robotkernel
gen_headerdir = $(top_builddir)/include/robotkernel

bin_PROGRAMS = robotkernel robotkernel_debug
include_HEADERS = $(headerdir)/module_intf.h \
				  $(headerdir)/module.h \
				  $(headerdir)/module_base.h \
				  $(headerdir)/interface.h \
				  $(headerdir)/exceptions.h \
				  $(headerdir)/kernel.h \
				  $(headerdir)/kernel_worker.h \
				  $(headerdir)/runnable.h \
				  $(headerdir)/trigger_base.h \
				  $(headerdir)/log_thread.h \
				  $(headerdir)/dump_log.h \
				  $(headerdir)/interface_intf.h \
				  $(headerdir)/helpers.h \
				  $(headerdir)/char_ringbuffer.h \
				  $(headerdir)/cmd_delay.h \
				  $(headerdir)/kernel_queue.h \
				  $(gen_headerdir)/config.h \
				  $(gen_headerdir)/ln_kernel_messages.h 

robotkernel_SOURCES = module.cpp module_intf.cpp interface.cpp main.cpp exceptions.cpp kernel.cpp kernel_worker.cpp \
					  runnable.cpp trigger_base.cpp log_thread.cpp dump_log.cpp char_ringbuffer.cpp

robotkernel_CXXFLAGS = -O2 -I$(top_builddir)/include -I$(top_srcdir)/include/ @YAMLCPP_CFLAGS@ @LN_CFLAGS@
robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic @ROBOTKERNEL_LDFLAGS@
robotkernel_LDADD = @DL_LIBS@ @SOCKET_LIBS@ @PTHREAD_LIBS@ @RT_LIBS@ @YAMLCPP_LIBS@ @LN_LIBS@ @STRING_UTILS_LIBS@ #-lstdc++

robotkernel_debug_SOURCES = module.cpp module_intf.cpp interface.cpp main.cpp exceptions.cpp kernel.cpp kernel_worker.cpp \
					  runnable.cpp trigger_base.cpp log_thread.cpp dump_log.cpp char_ringbuffer.cpp

robotkernel_debug_CXXFLAGS = -O0 -g3 -I$(top_builddir)/include -I$(top_srcdir)/include/ @YAMLCPP_CFLAGS@ @LN_CFLAGS@
robotkernel_debug_LDFLAGS = -Wl,-export-dynamic -Bdynamic @ROBOTKERNEL_LDFLAGS@
robotkernel_debug_LDADD = @DL_LIBS@ @SOCKET_LIBS@ @RT_LIBS@ @PTHREAD_LIBS@ @YAMLCPP_LIBS@ @LN_LIBS@ @STRING_UTILS_LIBS@ #-lstdc++

if HAVE_LTTNG_UST
robotkernel_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_LDADD += @LTTNG_UST_LIBS@
robotkernel_SOURCES += dump_log_tp.c

robotkernel_debug_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_debug_LDADD += @LTTNG_UST_LIBS@
robotkernel_debug_SOURCES += dump_log_tp.c

BUILT_SOURCES += dump_log_tp.h dump_log_tp.c
endif

VPATH = $(srcdir):$(top_srcdir)/share/ln/message_definitions 

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


