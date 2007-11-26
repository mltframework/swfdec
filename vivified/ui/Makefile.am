noinst_PROGRAMS = vivified

vivified_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_GTK_CFLAGS) $(VIVI_CFLAGS)
vivified_LDFLAGS = $(VIVI_LIBS) $(SWFDEC_GTK_LIBS)
vivified_LDADD = \
	$(top_builddir)/vivified/core/libvivified-core.la \
	$(top_builddir)/vivified/dock/libvivified-dock.la

vivified_SOURCES = \
	vivi_command_line.c \
	vivi_movie_list.c \
	vivi_movies.c \
	vivi_player.c \
	vivi_vivi_docklet.c \
	vivi_widget.c \
	vivi_window.c \
	main.c

noinst_HEADERS = \
	vivi_movie_list.h \
	vivi_vivi_docklet.h \
	vivi_widget.h \
	vivi_window.h

