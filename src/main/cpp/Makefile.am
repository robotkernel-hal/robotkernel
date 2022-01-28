INSTALL_HEADER = $(INSTALL_DATA) -C

lib_LTLIBRARIES = librobotkernel.la libservice_definitions.la

BUILT_SOURCES = $(top_builddir)/src/main/cpp/include/robotkernel/service_definitions.h
CLEANFILES = $(top_builddir)/src/main/cpp/include/robotkernel/service_definitions.h

includedir = $(prefix)/include/robotkernel
headerdir = $(srcdir)/include/robotkernel
gen_headerdir = $(top_builddir)/src/main/cpp/include/robotkernel

bin_PROGRAMS = robotkernel
include_HEADERS = $(headerdir)/bridge_base.h	\
				  $(headerdir)/bridge.h \
				  $(headerdir)/bridge_intf.h \
				  $(headerdir)/char_ringbuffer.h \
				  $(headerdir)/config.h.in \
				  $(headerdir)/device.h \
				  $(headerdir)/device_listener.h \
				  $(headerdir)/dump_log.h \
				  $(headerdir)/exceptions.h \
				  $(headerdir)/helpers.h \
				  $(headerdir)/interface_intf.h \
				  $(headerdir)/kernel.h \
				  $(headerdir)/kernel_c_wrapper.h \
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

librobotkernel_la_SOURCES = bridge.cpp				\
					  char_ringbuffer.cpp 		\
					  dump_log.cpp 				\
					  exceptions.cpp 			\
					  kernel.cpp 				\
					  module_base.cpp 			\
					  kernel_c_wrapper.cpp      \
					  kernel_worker.cpp       	\
					  log_base.cpp 				\
					  log_thread.cpp 			\
					  module.cpp				\
					  process_data.cpp			\
					  rk_type.cpp				\
					  rt_helper.cpp 			\
					  runnable.cpp 				\
					  service_provider.cpp 		\
					  so_file.cpp 				\
					  stream.cpp                \
					  trigger.cpp               \
					  trigger_worker.cpp        \
					  helpers.cpp				\
					  rkc_loader.cpp			\
					  service_definitions.cpp

librobotkernel_la_CXXFLAGS = -Wno-unused-local-typedefs 					\
					   -I$(top_builddir)/src/main/cpp/include 		\
					   -I$(srcdir)/include/ 						\
					   @YAML_CPP_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ 

librobotkernel_la_LDFLAGS=-static

VPATH = $(srcdir):$(top_srcdir)/share/robotkernel/service_definitions

SERVICE_DEFINITIONS = module/set_state	\
					  module/get_state	\
					  module/get_config \
					  kernel/get_dump_log \
					  kernel/config_dump_log \
					  kernel/add_module \
					  kernel/remove_module \
					  kernel/module_list \
					  kernel/reconfigure_module \
					  kernel/list_devices \
					  kernel/process_data_info \
					  kernel/trigger_info \
					  kernel/stream_info \
					  kernel/service_interface_info \
					  kernel/add_pd_injection \
					  kernel/del_pd_injection \
					  kernel/list_pd_injections \
					  log_base/configure_loglevel

$(top_builddir)/src/main/cpp/include/robotkernel/service_definitions.h: Makefile $(SERVICE_DEFINITIONS)
	install -d $(top_builddir)/src/main/cpp/include/robotkernel
	@echo creating $@
	service_generate --indir $(top_srcdir)/share/robotkernel/service_definitions --out $@ $(SERVICE_DEFINITIONS)
	touch $@

robotkernel_SOURCES = main.cpp

robotkernel_CXXFLAGS = -Wno-unused-local-typedefs 					\
					   -I$(top_builddir)/src/main/cpp/include 		\
					   -I$(srcdir)/include/ 						\
					   @YAML_CPP_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@ 

robotkernel_LDFLAGS = -Wl,-export-dynamic -Bdynamic -Wl,--whole-archive,.libs/librobotkernel.a,--no-whole-archive
robotkernel_LDADD = librobotkernel.la @PTHREAD_LIBS@ @YAML_CPP_LIBS@ @LIBSTRING_UTIL_LIBS@

libservice_definitions_la_SOURCES  = service_definitions.cpp
libservice_definitions_la_CXXFLAGS = -Wno-unused-local-typedefs 			\
					   -I$(top_builddir)/src/main/cpp/include 		\
					   -I$(srcdir)/include/ 						\
					   @YAML_CPP_CFLAGS@ @LIBSTRING_UTIL_CFLAGS@  
libservice_definitions_la_LDFLAGS  = 

if HAVE_LTTNG_UST
robotkernel_CXXFLAGS += @LTTNG_UST_CFLAGS@
robotkernel_LDADD += @LTTNG_UST_LIBS@
endif

