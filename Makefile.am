if WITH_GTK
GTK_SUBDIRS = \
	libswfdec-gtk \
	doc \
	player
else
GTK_SUBDIRS =
endif

if HAVE_VIVI
VIVI_SUBDIRS = vivified
else
VIVI_SUBDIRS =
endif

SUBDIRS= \
	libswfdec \
	$(GTK_SUBDIRS) \
	$(VIVI_SUBDIRS) \
	test

ACLOCAL_FLAGS = -I m4

EXTRA_DIST=depcomp \
	AUTHORS \
	COPYING \
	README \
	ChangeLog \
	autogen.sh \
	gtk-doc.make \
	swfdec.spec \
	m4/as-compiler-flag.m4 \
	m4/ax_create_stdint_h.m4 \
	m4/gtk-doc.m4

pkgconfigdir = $(libdir)/pkgconfig
if WITH_GTK
pkgconfig_DATA = swfdec-@SWFDEC_MAJORMINOR@.pc swfdec-gtk-@SWFDEC_MAJORMINOR@.pc
else
pkgconfig_DATA = swfdec-@SWFDEC_MAJORMINOR@.pc
endif

CLEANFILES = swfdec-$(SWFDEC_MAJORMINOR).pc swfdec-gtk-$(SWFDEC_MAJORMINOR).pc

swfdec-@SWFDEC_MAJORMINOR@.pc: swfdec.pc
	cp swfdec.pc swfdec-@SWFDEC_MAJORMINOR@.pc

swfdec-gtk-@SWFDEC_MAJORMINOR@.pc: swfdec-gtk.pc
	cp swfdec-gtk.pc swfdec-gtk-@SWFDEC_MAJORMINOR@.pc

# build documentation when doing distcheck
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

