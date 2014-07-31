ACLOCAL_AMFLAGS = -I m4 

lib_LTLIBRARIES = libmodule_example.la

libmodule_example_la_SOURCES  = module_example.cpp
libmodule_example_la_CXXFLAGS = -Wall -I$(top_srcdir) -I$(top_builddir) @ROBOTKERNEL_CFLAGS@ @LN_CFLAGS@ @YAMLCPP_CFLAGS@
libmodule_example_la_LDFLAGS  = -Wall
