ACLOCAL_AMFLAGS = -I m4 -I scripts/m4 

SUBDIRS = src

install-exec-hook:
	mkdir -p $(prefix)/share/ln
	-cp -rf $(srcdir)/share/ln/* $(prefix)/share/ln/

