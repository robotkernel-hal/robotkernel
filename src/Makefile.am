INSTALL_HEADER = $(INSTALL_DATA) -C

lib_LTLIBRARIES = librobotkernel.la

BUILT_SOURCES = $(top_builddir)/include/robotkernel/service_definitions.h
CLEANFILES = $(top_builddir)/include/robotkernel/service_definitions.h

includedir = $(prefix)/include/robotkernel
headerdir = $(top_builddir)/include/robotkernel
gen_headerdir = $(top_builddir)/include/robotkernel


#				  $(headerdir)/bridge.h
#				  $(headerdir)/kernel.h
#				  $(headerdir)/kernel_worker.h
#				  $(headerdir)/log_thread.h
#				  $(headerdir)/module.h
#				  $(headerdir)/service_provider.h
#				  $(headerdir)/so_file.h
#				  $(headerdir)/char_ringbuffer.h
#				  $(headerdir)/dump_log.h
#				  $(headerdir)/module_intf.h
#				$(headerdir)/bridge_intf.h 

bin_PROGRAMS = robotkernel
include_HEADERS = $(headerdir)/bridge_base.h	\
				  $(headerdir)/config.h.in \
				  $(headerdir)/device.h \
				  $(headerdir)/device_listener.h \
				  $(headerdir)/exceptions.h \
				  $(headerdir)/helpers.h \
				  $(headerdir)/robotkernel.h \
				  $(headerdir)/kernel_c_wrapper.h \
				  $(headerdir)/log_base.h \
				  $(headerdir)/loglevel.h \
				  $(headerdir)/module_base.h \
				  $(headerdir)/process_data.h \
				  $(headerdir)/rk_type.h \
				  $(headerdir)/runnable.h \
				  $(headerdir)/service.h \
				  $(headerdir)/service_definitions.h \
				  $(headerdir)/service_provider_base.h \
				  $(headerdir)/service_interface.h \
				  $(headerdir)/stream.h \
				  $(headerdir)/trigger_base.h \
				  $(headerdir)/trigger.h \
				  $(headerdir)/trigger_collector.h \
				  $(headerdir)/trigger_worker.h \
				  $(gen_headerdir)/config.h

librobotkernel_la_SOURCES = bridge.cpp				\
					  char_ringbuffer.cpp 		\
					  dump_log.cpp 				\
					  exceptions.cpp 			\
					  kernel.cpp 				\
					  robotkernel.cpp 				\
					  module_base.cpp 			\
					  kernel_c_wrapper.cpp      \
					  kernel_worker.cpp       	\
					  log_base.cpp 				\
					  log_thread.cpp 			\
					  module.cpp				\
					  process_data.cpp			\
					  rk_type.cpp				\
					  runnable.cpp 				\
					  service_provider.cpp 		\
					  so_file.cpp 				\
					  stream.cpp                \
					  trigger.cpp               \
					  trigger_worker.cpp        \
					  helpers.cpp				\
					  rkc_loader.cpp

librobotkernel_la_CXXFLAGS = -Wno-unused-local-typedefs 					\
					   -I$(top_builddir)/include 		\
					   -I$(srcdir) 						\
					   @YAML_CPP_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ 

librobotkernel_la_LDFLAGS=-static

VPATH = $(srcdir):$(top_srcdir)/share/service_definitions

SERVICE_DEFINITIONS = robotkernel/module/set_state	\
					  robotkernel/module/get_state	\
					  robotkernel/module/get_config \
					  robotkernel/kernel/get_dump_log \
					  robotkernel/kernel/config_dump_log \
					  robotkernel/kernel/add_module \
					  robotkernel/kernel/remove_module \
					  robotkernel/kernel/module_list \
					  robotkernel/kernel/reconfigure_module \
					  robotkernel/kernel/list_devices \
					  robotkernel/kernel/process_data_info \
					  robotkernel/kernel/trigger_info \
					  robotkernel/kernel/stream_info \
					  robotkernel/kernel/service_interface_info \
					  robotkernel/kernel/add_pd_injection \
					  robotkernel/kernel/del_pd_injection \
					  robotkernel/kernel/list_pd_injections \
					  robotkernel/log_base/configure_loglevel

$(top_builddir)/include/robotkernel/service_definitions.h: Makefile $(SERVICE_DEFINITIONS)
	install -d $(top_builddir)/include/robotkernel
	@echo creating $@
	service_generate --add_namespace services --indir $(top_srcdir)/share/service_definitions --out $@ $(SERVICE_DEFINITIONS)
	touch $@


robotkernel_SOURCES = main.cpp

robotkernel_CXXFLAGS = -Wno-unused-local-typedefs 					\
					   -I$(top_builddir)/include 		\
					   -I$(srcdir) 						\
					   @YAML_CPP_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ 

robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic -Wl,--whole-archive,.libs/librobotkernel.a,--no-whole-archive
robotkernel_LDADD = librobotkernel.la @PTHREAD_LIBS@ @YAML_CPP_LIBS@ @LIBSTRING_UTIL_LIBS@

if HAVE_LTTNG_UST
robotkernel_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_LDADD += @LTTNG_UST_LIBS@
endif

