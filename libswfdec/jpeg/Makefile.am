

noinst_LTLIBRARIES = libjpeg.la

libjpeg_la_SOURCES = jpeg.c huffman.c
libjpeg_la_CFLAGS = $(GLIB_CFLAGS) $(LIBART_CFLAGS) $(SWF_CFLAGS)
libjpeg_la_LDFLAGS = -lm $(GLIB_LIBS)

noinst_HEADERS = jpeg.h

noinst_PROGRAMS = jpeg_test

jpeg_test_SOURCES = test.c
jpeg_test_CFLAGS = $(GLIB_CFLAGS) $(LIBART_CFLAGS) $(SWF_CFLAGS)
jpeg_test_LDFLAGS = -lm $(GLIB_LIBS) $(LIBART_LIBS) $(SWF_LIBS)
jpeg_test_LDADD = libjpeg.la

