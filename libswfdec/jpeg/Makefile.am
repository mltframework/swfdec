

noinst_LTLIBRARIES = libjpeg.la

libjpeg_la_SOURCES = jpeg.c huffman.c jpeg_rgb_decoder.c
libjpeg_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(GLIB_CFLAGS) $(LIBOIL_CFLAGS) \
	-DG_LOG_DOMAIN=\"Swfdec\"
libjpeg_la_LDFLAGS = -lm

noinst_HEADERS = bits.h huffman.h jpeg.h jpeg_debug.h \
	jpeg_internal.h jpeg_rgb_decoder.h jpeg_rgb_internal.h
	

noinst_PROGRAMS = jpeg_test jpeg_rgb_test

jpeg_test_SOURCES = test.c
jpeg_test_CFLAGS = $(GLIB_CFLAGS)
jpeg_test_LDFLAGS = -lm $(LIBOIL_LIBS)
jpeg_test_LDADD = libjpeg.la $(GLIB_LIBS)

jpeg_rgb_test_SOURCES = test_rgb.c
jpeg_rgb_test_CFLAGS = $(GLIB_CFLAGS)
jpeg_rgb_test_LDFLAGS = -lm $(LIBOIL_LIBS)
jpeg_rgb_test_LDADD = libjpeg.la $(GLIB_LIBS)

