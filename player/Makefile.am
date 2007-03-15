if WITH_GTK

# this workaround is needed or autotools don't generate the .deps/*.Plo files correctly
swfdec_playback.c: swfdec_playback_$(AUDIO_TYPE).c Makefile
	cmp -s $(srcdir)/swfdec_playback_$(AUDIO_TYPE).c swfdec_playback.c \
	|| cp $(srcdir)/swfdec_playback_$(AUDIO_TYPE).c swfdec_playback.c

BUILT_SOURCES = swfdec_playback.c
CLEANFILES = swfdec_playback.c

noinst_LTLIBRARIES = libswfdecui.la
noinst_PROGRAMS = swfplay swfdebug

libswfdecui_la_SOURCES = \
	swfdec_playback.c \
	swfdec_source.c \
	swfdec_widget.c

swfplay_SOURCES = \
	swfdec_slow_loader.c \
	swfplay.c 

swfdebug_SOURCES = \
	swfdebug.c \
	swfdec_debug_movies.c \
	swfdec_debug_script.c \
	swfdec_debug_scripts.c \
	swfdec_debug_stack.c \
	swfdec_player_manager.c \
	swfdec_debug_widget.c

noinst_HEADERS = \
	swfdec_debug_movies.h \
	swfdec_debug_script.h \
	swfdec_debug_scripts.h \
	swfdec_debug_stack.h \
	swfdec_debug_widget.h \
	swfdec_playback.h \
	swfdec_player_manager.h \
	swfdec_source.h \
	swfdec_slow_loader.h \
	swfdec_widget.h 

libswfdecui_la_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) $(AUDIO_CFLAGS)
libswfdecui_la_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS) $(AUDIO_LIBS)
swfplay_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS)
swfplay_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS)
swfplay_LDADD = libswfdecui.la
swfdebug_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfdebug_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS)
swfdebug_LDADD = libswfdecui.la $(top_builddir)/libswfdec/js/libjs.la
endif

EXTRA_DIST = \
	swfdec_playback_alsa.c \
	swfdec_playback_none.c

