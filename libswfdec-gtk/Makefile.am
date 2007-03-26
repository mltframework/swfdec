if WITH_GTK

# this workaround is needed or autotools don't generate the .deps/*.Plo files correctly
swfdec_playback.c: swfdec_playback_$(AUDIO_TYPE).c Makefile
	cmp -s $(srcdir)/swfdec_playback_$(AUDIO_TYPE).c swfdec_playback.c \
	|| cp $(srcdir)/swfdec_playback_$(AUDIO_TYPE).c swfdec_playback.c

BUILT_SOURCES = swfdec_playback.c
CLEANFILES = swfdec_playback.c

lib_LTLIBRARIES = libswfdec-gtk-@SWFDEC_MAJORMINOR@.la

libswfdec_gtk_@SWFDEC_MAJORMINOR@_la_SOURCES = \
	swfdec_playback.c \
	swfdec_source.c \
	swfdec_widget.c

noinst_HEADERS = \
	swfdec_playback.h \
	swfdec_source.h \
	swfdec_widget.h

libswfdec_gtk_@SWFDEC_MAJORMINOR@_la_CFLAGS = \
	$(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS) $(AUDIO_CFLAGS) \
	-DG_LOG_DOMAIN=\"Swfdec-Gtk\"
libswfdec_gtk_@SWFDEC_MAJORMINOR@_la_LDFLAGS = \
	-version-info $(SWFDEC_LIBVERSION) \
	-export-symbols-regex '^(swfdec_.*)' \
	$(GTK_LIBS) $(SWFDEC_LIBS) $(AUDIO_LIBS)

endif

EXTRA_DIST = \
	swfdec_playback_alsa.c \
	swfdec_playback_none.c
