noinst_PROGRAMS = swfplay 

swfplay_SOURCES = \
	swfdec_slow_loader.c \
	swfplay.c 

#swfdebug_SOURCES = \
#	swfdebug.c \
#	swfdec_debug_script.c \
#	swfdec_debug_scripts.c \
#	swfdec_debug_stack.c \
#	swfdec_player_manager.c \
#	swfdec_debug_widget.c

noinst_HEADERS = \
	swfdec_slow_loader.h

swfplay_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWFDEC_GTK_CFLAGS)
swfplay_LDFLAGS = $(SWFDEC_GTK_LIBS) $(GTK_LIBS)

