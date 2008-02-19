

noinst_LTLIBRARIES = libjpeg.la

libjpeg_la_SOURCES = \
	jpeg.c \
	jpeg_bits.c \
	jpeg_huffman.c \
	jpeg_rgb_decoder.c \
	jpeg_tables.c

noinst_HEADERS = \
	cogcompat.h \
	jpeg.h \
	jpeg_bits.h \
	jpeg_huffman.h \
	jpeg_rgb_decoder.h

libjpeg_la_CFLAGS = $(GLOBAL_CFLAGS) $(GLIB_CFLAGS) $(LIBOIL_CFLAGS) -I$(srcdir)/..

