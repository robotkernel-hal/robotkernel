ACLOCAL_AMFLAGS = -I m4 -I scripts/m4 

SUBDIRS = src/main/cpp
pkgconfig_DATA = robotkernel.pc

install-exec-hook:
	mkdir -p $(prefix)/share/ln
	-cp -rf $(srcdir)/share/ln/* $(prefix)/share/ln/

