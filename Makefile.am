ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src/main/cpp
pkgconfig_DATA = robotkernel.pc

install-exec-hook:
	mkdir -p $(prefix)/share/ln
	-cp -rf $(top_builddir)/share/ln/* $(prefix)/share/ln/
	-cp $(top_srcdir)/share/ln/generate_ln_includes $(DESTDIR)$(prefix)/share/ln/

