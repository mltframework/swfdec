if WITH_GTK
if WITH_ALSA
noinst_PROGRAMS = swfplay swfdebug

swfplay_SOURCES = swfplay.c swfdec_widget.c swfdec_playback.c
swfdebug_SOURCES = swfdebug.c swfdec_widget.c

noinst_HEADERS = swfdec_widget.h swfdec_playback.h

swfplay_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) $(ALSA_CFLAGS)
swfplay_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS) $(ALSA_LIBS)
swfdebug_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) $(ALSA_CFLAGS)
swfdebug_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS) $(ALSA_LIBS)
endif
endif

EXTRA_DIST = swfdebug.c swfplay.c swfdec_widget.c swfdec_widget.h swfdec_playback.c swfdec_playback.h
