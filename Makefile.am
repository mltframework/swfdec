SUBDIRS= \
	gimp-loader \
	libswfdec \
	pixbuf-loader \
	player \
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

