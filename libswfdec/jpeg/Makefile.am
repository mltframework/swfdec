

noinst_LTLIBRARIES = libjpeg.la

libjpeg_la_SOURCES = jpeg.c huffman.c jpeg_rgb_decoder.c
libjpeg_la_CFLAGS = $(SWF_CFLAGS) $(GLIB_CFLAGS) $(LIBOIL_CFLAGS)
libjpeg_la_LDFLAGS = -lm $(GLIB_LIBS)

noinst_HEADERS = bits.h conv8x8_f64_s16.h huffman.h idct8_f64.h \
	idct8x8_f64.h idct8x8_s16.h jpeg.h jpeg_debug.h \
	jpeg_internal.h jpeg_rgb_decoder.h jpeg_rgb_internal.h \
	sl_block8x8.h sl_types.h unzigzag8x8_s16.h

noinst_PROGRAMS = jpeg_test jpeg_rgb_test

jpeg_test_SOURCES = test.c
jpeg_test_CFLAGS = $(GLIB_CFLAGS)
jpeg_test_LDFLAGS = -lm $(GLIB_LIBS) $(LIBOIL_LIBS)
jpeg_test_LDADD = libjpeg.la

jpeg_rgb_test_SOURCES = test_rgb.c
jpeg_rgb_test_CFLAGS = $(GLIB_CFLAGS)
jpeg_rgb_test_LDFLAGS = -lm $(GLIB_LIBS) $(LIBOIL_LIBS)
jpeg_rgb_test_LDADD = libjpeg.la

