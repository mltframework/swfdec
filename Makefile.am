SUBDIRS=libswfdec test player plugin pixbuf-loader gimp-loader

DIST_SUBDIRS=libswfdec test player plugin pixbuf-loader gimp-loader

EXTRA_DIST=depcomp AUTHORS COPYING README RELEASE ChangeLog autogen.sh swfdec.spec \
	TODO m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = swfdec.pc


dist-hook:
	cp swfdec.spec $(distdir)
	mkdir $(distdir)/debian
	-cp debian/* $(distdir)/debian
	rm -rf `find $(distdir)/m4 -name CVS`

