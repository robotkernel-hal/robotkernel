INSTALL_HEADER = $(INSTALL_DATA) -C

BUILT_SOURCES=
includedir = $(prefix)/include/robotkernel
headerdir = $(srcdir)/include/robotkernel
gen_headerdir = $(top_builddir)/src/main/cpp/include/robotkernel

bin_PROGRAMS = robotkernel
include_HEADERS = $(headerdir)/bridge_base.h	\
				  $(headerdir)/bridge.h \
				  $(headerdir)/bridge_intf.h \
				  $(headerdir)/char_ringbuffer.h \
				  $(headerdir)/cmd_delay.h \
				  $(headerdir)/config.h.in \
				  $(headerdir)/device.h \
				  $(headerdir)/dump_log.h \
				  $(headerdir)/exceptions.h \
				  $(headerdir)/helpers.h \
				  $(headerdir)/interface_intf.h \
				  $(headerdir)/kernel.h \
				  $(headerdir)/kernel_queue.h \
				  $(headerdir)/kernel_worker.h \
				  $(headerdir)/log_base.h \
				  $(headerdir)/loglevel.h \
				  $(headerdir)/log_thread.h \
				  $(headerdir)/module_base.h \
				  $(headerdir)/module.h \
				  $(headerdir)/module_intf.h \
				  $(headerdir)/process_data.h \
				  $(headerdir)/rk_type.h \
				  $(headerdir)/rt_helper.h \
				  $(headerdir)/runnable.h \
				  $(headerdir)/service.h \
				  $(headerdir)/service_provider_base.h \
				  $(headerdir)/service_provider.h \
				  $(headerdir)/service_provider_intf.h \
				  $(headerdir)/service_interface.h \
				  $(headerdir)/so_file.h \
				  $(headerdir)/stream.h \
				  $(headerdir)/trigger_base.h \
				  $(headerdir)/trigger.h \
				  $(headerdir)/trigger_worker.h \
				  $(gen_headerdir)/config.h

robotkernel_SOURCES = bridge.cpp				\
					  char_ringbuffer.cpp 		\
					  dump_log.cpp 				\
					  exceptions.cpp 			\
					  kernel.cpp 				\
					  kernel_worker.cpp       	\
					  log_base.cpp 				\
					  log_thread.cpp 			\
					  main.cpp 					\
					  module.cpp				\
					  rk_type.cpp				\
					  rt_helper.cpp 			\
					  runnable.cpp 				\
					  service_provider.cpp 		\
					  so_file.cpp 				\
					  trigger.cpp               \
					  trigger_worker.cpp

robotkernel_CXXFLAGS = -Wno-unused-local-typedefs 					\
					   -I$(top_builddir)/src/main/cpp/include 		\
					   -I$(srcdir)/include/ 						\
					   @YAML_CPP_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ 

robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic
robotkernel_LDADD = @PTHREAD_LIBS@ @YAML_CPP_LIBS@ @LIBSTRING_UTIL_LIBS@


if HAVE_LTTNG_UST
robotkernel_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_LDADD += @LTTNG_UST_LIBS@
robotkernel_SOURCES += dump_log_tp.c

BUILT_SOURCES += dump_log_tp.h dump_log_tp.c
endif

dump_log_tp.c dump_log_tp.h: dump_log_tp.tp
	@echo creating $@ from $^
	lttng-gen-tp -o $@ $^


