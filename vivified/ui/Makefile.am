noinst_PROGRAMS = vivified

vivified_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_GTK_CFLAGS)
vivified_LDFLAGS = $(SWFDEC_GTK_LIBS) \
	$(top_builddir)/vivified/core/libvivified-core.la \
	$(top_builddir)/vivified/dock/libvivified-dock.la

vivified_SOURCES = \
	vivi_commandline.c \
	main.c

noinst_HEADERS = \
	vivi_commandline.h

