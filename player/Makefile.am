
if WITH_GTK
if WITH_SDL
bin_PROGRAMS = swf_play
else
bin_PROGRAMS = 
endif
else
bin_PROGRAMS = 
endif

swf_play_CFLAGS = $(SWF_CFLAGS) $(GTK_CFLAGS) $(LIBART_CFLAGS) $(GLIB_CFLAGS) $(SDL_CFLAGS)
swf_play_LDFLAGS = $(SWF_LIBS) $(GTK_LIBS) $(LIBART_LIBS) $(GLIB_LIBS) $(SDL_LIBS)

