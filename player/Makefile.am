if WITH_GTK

noinst_PROGRAMS = swfplay swfdebug

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
	swfdec_player_manager.h \
	swfdec_slow_loader.h

swfplay_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWFDEC_GTK_CFLAGS)
swfplay_LDFLAGS = $(GTK_LIBS) $(SWFDEC_GTK_LIBS)

swfdebug_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWFDEC_GTK_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfdebug_LDFLAGS = $(GTK_LIBS) $(SWFDEC_GTK_LIBS)
swfdebug_LDADD = $(top_builddir)/libswfdec/js/libjs.la

endif


