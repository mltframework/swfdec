
if WITH_GLIB_COMPAT
glib_compat_subdirs = glib-compat
#no_glib_compat_subdirs =
else
glib_compat_subdirs =
endif
no_glib_compat_subdirs = gimp-loader pixbuf-loader player plugin

SUBDIRS=$(glib_compat_subdirs) libswfdec test $(no_glib_compat_subdirs)

DIST_SUBDIRS= \
	gimp-loader \
	glib-compat \
	libswfdec \
	pixbuf-loader \
	player \
	plugin \
	test

EXTRA_DIST=depcomp AUTHORS COPYING README RELEASE ChangeLog autogen.sh swfdec.spec \
	TODO m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = swfdec-@SWFDEC_MAJORMINOR@.pc

EXTRA_CLEAN=swfdec.pc

swfdec-@SWFDEC_MAJORMINOR@.pc: swfdec.pc
	cp swfdec.pc swfdec-@SWFDEC_MAJORMINOR@.pc

dist-hook:
	cp swfdec.spec $(distdir)
	mkdir $(distdir)/debian
	-cp debian/* $(distdir)/debian
	rm -rf `find $(distdir)/m4 -name CVS`

