

CFLAGS=-Wall -g -DLAYER1 -DLAYER2 -DLAYER3

noinst_LTLIBRARIES = libmpglib.la

libmpglib_la_SOURCES = layer1.c layer2.c common.c dct64_i386.c \
	decode_i386.c layer3.c tabinit.c interface.c main.c
libmpglib_la_CFLAGS = -DLAYER1 -DLAYER2 -DLAYER3
libmpglib_la_LDFLAGS = -lm

noinst_HEADERS = huffman.h l2tables.h mpg123.h mpglib.h
