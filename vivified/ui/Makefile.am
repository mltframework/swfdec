noinst_PROGRAMS = vivified

vivified_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_GTK_CFLAGS)
vivified_LDFLAGS = $(SWFDEC_GTK_LIBS)
vivified_LDADD = \
	$(top_builddir)/vivified/core/libvivified-core.la \
	$(top_builddir)/vivified/dock/libvivified-dock.la

vivified_SOURCES = \
	vivi_commandline.c \
	vivi_movie_list.c \
	vivi_player.c \
	main.c

noinst_HEADERS = \
	vivi_commandline.h \
	vivi_movie_list.h \
	vivi_player.h

