

noinst_LTLIBRARIES = libjpeg.la

libjpeg_la_SOURCES = jpeg.c huffman.c jpeg_rgb_decoder.c
libjpeg_la_CFLAGS = $(GLIB_CFLAGS)
libjpeg_la_LDFLAGS = -lm $(GLIB_LIBS)

noinst_HEADERS = jpeg.h

noinst_PROGRAMS = jpeg_test jpeg_rgb_test

jpeg_test_SOURCES = test.c
jpeg_test_CFLAGS = $(GLIB_CFLAGS)
jpeg_test_LDFLAGS = -lm $(GLIB_LIBS)
jpeg_test_LDADD = libjpeg.la

jpeg_rgb_test_SOURCES = test_rgb.c
jpeg_rgb_test_CFLAGS = $(GLIB_CFLAGS)
jpeg_rgb_test_LDFLAGS = -lm $(GLIB_LIBS)
jpeg_rgb_test_LDADD = libjpeg.la

