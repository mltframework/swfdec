# these are tools used in creating these audio files
noinst_PROGRAMS = cut-silence downsample

check_PROGRAMS = sound
TESTS = $(check_PROGRAMS)

cut_silence_SOURCES = cut_silence.c
cut_silence_CFLAGS = $(GLOBAL_CFLAGS) $(GLIB_CFLAGS)
cut_silence_LDFLAGS = $(GLIB_LIBS)

downsample_SOURCES = downsample.c
downsample_CFLAGS = $(GLOBAL_CFLAGS) $(GLIB_CFLAGS)
downsample_LDFLAGS = $(GLIB_LIBS)

sound_SOURCES = sound.c
sound_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
sound_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

EXTRA_DIST = \
	README \
	adpcm-2.swf \
	adpcm-2.swf.1.0.raw \
	adpcm-2-2.swf \
	adpcm-2-2.swf.1.0.raw \
	adpcm-3.swf \
	adpcm-3.swf.1.0.raw \
	adpcm-3-2.swf \
	adpcm-3-2.swf.1.0.raw \
	adpcm-4.swf \
	adpcm-4.swf.1.0.raw \
	adpcm-4-2.swf \
	adpcm-4-2.swf.1.0.raw \
	adpcm-5.swf \
	adpcm-5.swf.1.0.raw \
	adpcm-5-2.swf \
	adpcm-5-2.swf.1.0.raw \
	crash-0.5.3-no-samples.c \
	crash-0.5.3-no-samples.swf \
	crash-0.5.3-no-samples.swf.1.0.raw

CLEANFILES = tmp
