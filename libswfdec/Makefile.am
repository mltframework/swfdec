
CFLAGS += -Werror

SUBDIRS = mpglib jpeg

lib_LTLIBRARIES = libswfdec.la

libswfdec_la_SOURCES = actions.c art.c color.c get.c jpeg.c \
	render.c shape.c sound.c swf.c swfdec_object.c text.c \
	swfdec_display.c swfdec_sprite.c swfdec_button.c swfdec_layer.c
libswfdec_la_CFLAGS = $(SWF_CFLAGS) $(LIBART_CFLAGS) $(GLIB_CFLAGS) \
	-I$(srcdir)/jpeg/
libswfdec_la_LDFLAGS = $(LIBART_LIBS) $(GLIB_LIBS) \
	-Wl,--version-script=version_script

libswfdec_la_LIBADD = mpglib/libmpglib.la jpeg/libjpeg.la

pkginclude_HEADERS = swf.h swfdec.h

noinst_HEADERS = art.h bits.h color.h get.h internal.h proto.h \
	swfdec_button.h swfdec_decoder.h swfdec_image.h \
	swfdec_internal.h swfdec_layer.h swfdec_object.h \
	swfdec_shape.h swfdec_sound.h swfdec_sprite.h \
	swfdec_text.h swfdec_types.h tags.h swfdec_display.h

