if WITH_GTK
if WITH_ALSA
noinst_PROGRAMS = swfplay swfdebug

swfplay_SOURCES = \
	swfdec_playback.c \
	swfdec_slow_loader.c \
	swfdec_source.c \
	swfdec_widget.c \
	swfplay.c 

swfdebug_SOURCES = \
	swfdebug.c \
	swfdec_debug_scripts.c \
	swfdec_widget.c

noinst_HEADERS = \
	swfdec_debug_scripts.h \
	swfdec_playback.h \
	swfdec_source.h \
	swfdec_slow_loader.h \
	swfdec_widget.h 

swfplay_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) $(ALSA_CFLAGS)
swfplay_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS) $(ALSA_LIBS)
swfdebug_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) $(ALSA_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfdebug_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS) $(ALSA_LIBS)
endif
endif

EXTRA_DIST = swfdebug.c swfplay.c swfdec_widget.c swfdec_widget.h swfdec_playback.c swfdec_playback.h
