
CFLAGS += -Werror

SUBDIRS = mpglib jpeg

lib_LTLIBRARIES = libswfdec.la

libswfdec_la_SOURCES = art.c get.c swf.c shape.c sound.c render.c actions.c text.c jpeg.c
libswfdec_la_CFLAGS = $(SWF_CFLAGS) $(LIBART_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS) -I jpeg/
libswfdec_la_LDFLAGS = $(LIBART_LIBS) $(GLIB_LIBS) $(GTK_LIBS)

libswfdec_la_LIBADD = mpglib/libmpglib.la jpeg/libjpeg.la

pkginclude_HEADERS = swf.h

noinst_HEADERS = internal.h proto.h bits.h tags.h

