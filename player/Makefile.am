if WITH_GTK
noinst_PROGRAMS = swfplay

swfplay_SOURCES = swfplay.c swfdec_widget.c

noinst_HEADERS = swfdec_widget.h

swfplay_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWF_CFLAGS) 
swfplay_LDFLAGS = $(GTK_LIBS) $(SWF_LIBS)
endif

