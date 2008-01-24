if WITH_GTK
GTK_SUBDIRS = \
	swfdec-gtk \
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
	swfdec \
	$(GTK_SUBDIRS) \
	$(VIVI_SUBDIRS) \
	data \
	test \
	tools

ACLOCAL_FLAGS = -I m4

EXTRA_DIST=depcomp \
	AUTHORS \
	COPYING \
	MAINTAINERS \
	README \
	ChangeLog \
	autogen.sh \
	gtk-doc.make \
	m4/as-compiler-flag.m4 \
	m4/ax_create_stdint_h.m4 \
	m4/gtk-doc.m4

# build documentation when doing distcheck
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

