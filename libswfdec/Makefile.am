
SUBDIRS = jpeg mpeg

lib_LTLIBRARIES = libswfdec-@SWFDEC_MAJORMINOR@.la

if HAVE_LIBART
renderer_cflags = $(LIBART_CFLAGS)
renderer_libs = $(LIBART_LIBS)
renderer_sources = swfdec_render_libart.c art.c
renderer_headers = swfdec_render_libart.h art.h
endif



libswfdec_@SWFDEC_MAJORMINOR@_la_SOURCES = actions.c color.c swfdec_image.c \
	render.c swfdec_render.c swfdec_shape.c swf.c swfdec_object.c swfdec_text.c \
	swfdec_display.c swfdec_sprite.c swfdec_button.c swfdec_layer.c \
	swfdec_sound.c swfdec_font.c swfdec_debug.c swfdec_bits.c \
	swfdec_buffer.c \
	swfdec_rect.c swfdec_transform.c $(renderer_sources)
libswfdec_@SWFDEC_MAJORMINOR@_la_CFLAGS = \
	$(SWF_CFLAGS) $(renderer_cflags) $(GLIB_CFLAGS) \
	-I$(srcdir)/jpeg/ $(LIBOIL_CFLAGS)
libswfdec_@SWFDEC_MAJORMINOR@_la_LDFLAGS = \
	$(renderer_libs) $(GLIB_LIBS) $(MAD_LIBS) \
	$(LIBOIL_LIBS)

libswfdec_@SWFDEC_MAJORMINOR@_la_LIBADD = jpeg/libjpeg.la

pkginclude_HEADERS = swf.h swfdec.h \
	color.h proto.h swfdec_buffer.h \
	swfdec_button.h swfdec_decoder.h swfdec_image.h \
	swfdec_internal.h swfdec_layer.h swfdec_object.h \
	swfdec_shape.h swfdec_sound.h swfdec_sprite.h \
	swfdec_text.h swfdec_types.h tags.h swfdec_display.h \
	swfdec_bits.h swfdec_rect.h swfdec_transform.h \
	swfdec_render.h \
	$(renderer_headers)

