ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src/main/cpp
pkgconfig_DATA = robotkernel.pc

install-exec-hook:
	mkdir -p $(DESTDIR)$(prefix)/share/ln
	-cp -rf $(top_builddir)/share/ln/* $(DESTDIR)$(prefix)/share/ln/

