

noinst_LTLIBRARIES = libmpglib.la

libmpglib_la_SOURCES = layer1.c layer2.c common.c dct64_i386.c \
	decode_i386.c layer3.c tabinit.c interface.c main.c
libmpglib_la_CFLAGS = -DLAYER1 -DLAYER2 -DLAYER3 $(GLIB_CFLAGS)
libmpglib_la_LDFLAGS = -lm $(GLIB_LIBS)

noinst_HEADERS = clipconv_f32_s16.h dct32_f32.h huffman.h l2tables.h \
	mpglib.h mpglib_config.h mpglib_internal.h

noinst_PROGRAMS = mpglib_test

mpglib_test_SOURCES = main.c
mpglib_test_CFLAGS = $(GLIB_CFLAGS)
mpglib_test_LDFLAGS = -lm $(GLIB_LIBS)
mpglib_test_LDADD = libmpglib.la


