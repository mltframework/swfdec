
SUBDIRS=libswfdec test plugin player

DIST_SUBDIRS=libswfdec test plugin player

EXTRA_DIST=depcomp AUTHORS COPYING README RELEASE ChangeLog autogen.sh \
	swfdec.pc.in TODO

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = swfdec.pc


dist-hook:
	cp swfdec.spec $(distdir)
	mkdir $(distdir)/debian
	-cp debian/* $(distdir)/debian

