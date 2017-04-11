INSTALL_HEADER = $(INSTALL_DATA) -C

BUILT_SOURCES=
includedir = $(prefix)/include/robotkernel
headerdir = $(srcdir)/include/robotkernel
gen_headerdir = $(top_builddir)/src/main/cpp/include/robotkernel

bin_PROGRAMS = robotkernel
include_HEADERS = $(headerdir)/module_intf.h \
				  $(headerdir)/module.h \
				  $(headerdir)/module_base.h \
				  $(headerdir)/service_provider.h \
				  $(headerdir)/service_provider_intf.h \
				  $(headerdir)/service_provider_base.h \
				  $(headerdir)/service_requester_base.h \
				  $(headerdir)/exceptions.h \
				  $(headerdir)/kernel.h \
				  $(headerdir)/kernel_worker.h \
				  $(headerdir)/runnable.h \
				  $(headerdir)/trigger_base.h \
				  $(headerdir)/log_thread.h \
				  $(headerdir)/loglevel.h \
				  $(headerdir)/dump_log.h \
				  $(headerdir)/interface_intf.h \
				  $(headerdir)/helpers.h \
				  $(headerdir)/char_ringbuffer.h \
				  $(headerdir)/cmd_delay.h \
				  $(headerdir)/kernel_queue.h \
				  $(headerdir)/service.h \
				  $(headerdir)/bridge_base.h \
				  $(headerdir)/bridge_intf.h \
				  $(headerdir)/log_base.h \
				  $(headerdir)/so_file.h \
				  $(headerdir)/rk_type.h \
				  $(gen_headerdir)/config.h

robotkernel_SOURCES = module.cpp module_intf.cpp service_provider.cpp main.cpp 	\
					  exceptions.cpp kernel.cpp kernel_worker.cpp       		\
					  runnable.cpp trigger_base.cpp log_thread.cpp 				\
					  dump_log.cpp char_ringbuffer.cpp module_base.cpp 			\
					  service_provider_base.cpp 								\
					  rt_helper.cpp rk_type.cpp									\
					  bridge_base.cpp log_base.cpp so_file.cpp bridge.cpp

robotkernel_CXXFLAGS = -Wno-unused-local-typedefs 					\
					   -I$(top_builddir)/src/main/cpp/include 		\
					   -I$(srcdir)/include/ 						\
					   @YAML_CPP_RMC_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ 

robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic
robotkernel_LDADD = @PTHREAD_LIBS@ @YAML_CPP_RMC_LIBS@ @LIBSTRING_UTIL_LIBS@


if HAVE_LTTNG_UST
robotkernel_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_LDADD += @LTTNG_UST_LIBS@
robotkernel_SOURCES += dump_log_tp.c

BUILT_SOURCES += dump_log_tp.h dump_log_tp.c
endif

dump_log_tp.c dump_log_tp.h: dump_log_tp.tp
	@echo creating $@ from $^
	lttng-gen-tp -o $@ $^


