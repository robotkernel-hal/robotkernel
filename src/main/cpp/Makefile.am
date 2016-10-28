INSTALL_HEADER = $(INSTALL_DATA) -C

BUILT_SOURCES = $(top_builddir)/src/main/cpp/include/robotkernel/ln_kernel_messages.h
CLEANFILES = $(top_builddir)/src/main/cpp/include/robotkernel/ln_kernel_messages.h
includedir = $(prefix)/include/robotkernel
headerdir = $(srcdir)/include/robotkernel
gen_headerdir = $(top_builddir)/src/main/cpp/include/robotkernel

bin_PROGRAMS = robotkernel robotkernel_debug
include_HEADERS = $(headerdir)/module_intf.h \
				  $(headerdir)/module.h \
				  $(headerdir)/module_base.h \
				  $(headerdir)/interface.h \
				  $(headerdir)/interface_base.h \
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
					  runnable.cpp trigger_base.cpp log_thread.cpp dump_log.cpp char_ringbuffer.cpp module_base.cpp interface_base.cpp

robotkernel_CXXFLAGS = -O2 -I$(top_builddir)/src/main/cpp/include -I$(srcdir)/include/ @YAML_CPP_CFLAGS@ @LN_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@
robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic @ROBOTKERNEL_LDFLAGS@
robotkernel_LDADD = @DL_LIBS@ @SOCKET_LIBS@ @PTHREAD_LIBS@ @RT_LIBS@ @YAML_CPP_LIBS@ @LN_LIBS@ @LIBSTRING_UTIL_LIBS@ #-lstdc++

robotkernel_debug_SOURCES = module.cpp module_intf.cpp interface.cpp main.cpp exceptions.cpp kernel.cpp kernel_worker.cpp \
					  runnable.cpp trigger_base.cpp log_thread.cpp dump_log.cpp char_ringbuffer.cpp module_base.cpp interface_base.cpp

robotkernel_debug_CXXFLAGS = -O0 -g3 -I$(top_builddir)/src/main/cpp/include -I$(srcdir)/include/ @YAML_CPP_CFLAGS@ @LN_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@
robotkernel_debug_LDFLAGS = -Wl,-export-dynamic -Bdynamic @ROBOTKERNEL_LDFLAGS@ 

robotkernel_debug_LDADD = @DL_LIBS@ @SOCKET_LIBS@ @RT_LIBS@ @PTHREAD_LIBS@ @YAML_CPP_LIBS@ @LN_LIBS@ @LIBSTRING_UTIL_LIBS@ #-lstdc++

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

LN_KERNEL_MESSAGES = \
					 robotkernel_add_module \
					 robotkernel_remove_module \
					 robotkernel_reconfigure_module \
					 robotkernel_set_state \
					 robotkernel_get_state \
					 robotkernel_get_states \
					 robotkernel_module_list \
					 robotkernel_get_dump_log \
					 robotkernel_config_dump_log \
					 robotkernel_module_get_feat \
					 robotkernel_module_get_config \
					 robotkernel_module_configure_loglevel \
					 robotkernel_module_get_state \
					 robotkernel_module_set_state 

dump_log_tp.c dump_log_tp.h: dump_log_tp.tp
	@echo creating $@ from $^
	lttng-gen-tp -o $@ $^

include $(top_srcdir)/Makefile.defs


