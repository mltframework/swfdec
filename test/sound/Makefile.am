# these are tools used in creating these audio files
noinst_PROGRAMS = cut-silence

check_PROGRAMS = sound
TESTS = $(check_PROGRAMS)

cut_silence_SOURCES = cut_silence.c
cut_silence_CFLAGS = $(GLOBAL_CFLAGS) $(GLIB_CFLAGS)
cut_silence_LDFLAGS = $(GLIB_LIBS)

sound_SOURCES = sound.c
sound_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS)
sound_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

#EXTRA_DIST = \
#	stopsounds.swf
#	stopsounds.wav

CLEANFILES = tmp
