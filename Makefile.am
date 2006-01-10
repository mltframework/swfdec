
if WITH_GLIB_COMPAT
glib_compat_subdirs = glib-compat
no_glib_compat_subdirs =
else
glib_compat_subdirs =
no_glib_compat_subdirs = gimp-loader pixbuf-loader
endif

SUBDIRS=$(glib_compat_subdirs) libswfdec test plugin $(no_glib_compat_subdirs)

DIST_SUBDIRS= \
	gimp-loader \
	glib-compat \
	libswfdec \
	pixbuf-loader \
	plugin \
	test

ACLOCAL_FLAGS = -I m4

EXTRA_DIST=depcomp AUTHORS COPYING README RELEASE ChangeLog autogen.sh swfdec.spec \
	TODO m4/ax_create_stdint_h.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = swfdec-@SWFDEC_MAJORMINOR@.pc

CLEANFILES = swfdec-$(SWFDEC_MAJORMINOR).pc

swfdec-@SWFDEC_MAJORMINOR@.pc: swfdec.pc
	cp swfdec.pc swfdec-@SWFDEC_MAJORMINOR@.pc

