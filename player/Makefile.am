
if WITH_SDL
bin_PROGRAMS = swf_play
else
bin_PROGRAMS = 
endif

swf_play_CFLAGS = $(SWF_CFLAGS) $(LIBART_CFLAGS) $(GLIB_CFLAGS) $(SDL_CFLAGS) -I ../plugin
swf_play_LDFLAGS = $(SWF_LIBS) $(LIBART_LIBS) $(GLIB_LIBS) $(SDL_LIBS)

