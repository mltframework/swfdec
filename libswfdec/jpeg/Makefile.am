

noinst_LTLIBRARIES = libjpeg.la
noinst_PROGRAMS = jpeg_test jpeg_rgb_test

libjpeg_la_SOURCES = jpeg.c huffman.c jpeg_rgb_decoder.c
libjpeg_la_CFLAGS = $(GLOBAL_CFLAGS) $(LIBOIL_CFLAGS)

noinst_HEADERS = bits.h huffman.h jpeg.h jpeg_debug.h \
	jpeg_internal.h
	


jpeg_test_SOURCES = test.c
jpeg_test_CFLAGS = $(LIBOIL_CFLAGS)
jpeg_test_LDADD = libjpeg.la $(LIBOIL_LIBS)

jpeg_rgb_test_SOURCES = test_rgb.c
jpeg_rgb_test_CFLAGS = $(LIBOIL_CFLAGS)
jpeg_rgb_test_LDADD = libjpeg.la $(LIBOIL_LIBS)

