SUBDIRS = jpeg js

DIST_SUBDIRS = jpeg js mpeg

lib_LTLIBRARIES = libswfdec-@SWFDEC_MAJORMINOR@.la

js_cflags = -I$(srcdir)/js/ -I./js -DXP_UNIX -DDEBUG

libswfdec_@SWFDEC_MAJORMINOR@_la_SOURCES = color.c \
	swfdec_rect.c swfdec_image.c swfdec_event.c \
	swfdec_shape.c swf.c swfdec_object.c swfdec_text.c swfdec_edittext.c swfdec_html_parser.c \
	swfdec_sprite.c swfdec_button.c swfdec_movieclip.c \
	swfdec_sound.c swfdec_font.c swfdec_debug.c swfdec_bits.c \
	swfdec_buffer.c swfdec_audio.c swfdec_tag.c \
	swfdec_cache.c swfdec_internal.h swfdec_decoder_internal.h \
	swfdec_js.c swfdec_js_global.c swfdec_js_mouse.c \
	swfdec_js_movieclip.c \
	swfdec_compiler.c 

libswfdec_@SWFDEC_MAJORMINOR@_la_CFLAGS = \
	$(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS) $(GLIB_CFLAGS) $(PANGO_CFLAGS) \
	-I$(srcdir)/jpeg/ $(js_cflags) $(LIBOIL_CFLAGS)
libswfdec_@SWFDEC_MAJORMINOR@_la_LDFLAGS = \
	-export-symbols-regex '^(swfdec_.*)' \
	$(CAIRO_LIBS) $(GLIB_LIBS) $(PANGO_LIBS) $(MAD_LIBS) \
	$(LIBOIL_LIBS)

libswfdec_@SWFDEC_MAJORMINOR@_la_LIBADD = jpeg/libjpeg.la js/libjs.la

libswfdec_@SWFDEC_MAJORMINOR@includedir = $(includedir)/swfdec-@SWFDEC_MAJORMINOR@

libswfdec_@SWFDEC_MAJORMINOR@include_HEADERS = swfdec.h \
	color.h swfdec_buffer.h swfdec_debug.h swfdec_event.h \
	swfdec_button.h swfdec_compiler.h swfdec_decoder.h swfdec_image.h \
	swfdec_object.h swfdec_font.h swfdec_movieclip.h \
	swfdec_shape.h swfdec_sound.h swfdec_sprite.h \
	swfdec_text.h swfdec_edittext.h swfdec_types.h tags.h \
	swfdec_bits.h swfdec_rect.h \
	swfdec_cache.h swfdec_audio.h \
	swfdec_js.h

