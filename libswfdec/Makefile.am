SUBDIRS = jpeg js

DIST_SUBDIRS = jpeg js mpeg

CODECS =

if HAVE_MAD
CODECS += swfdec_codec_mad.c
endif
if HAVE_FFMPEG
CODECS += swfdec_codec_ffmpeg.c
endif

lib_LTLIBRARIES = libswfdec-@SWFDEC_MAJORMINOR@.la

js_cflags = -I$(srcdir)/js/ -I./js -DXP_UNIX -DDEBUG

libswfdec_@SWFDEC_MAJORMINOR@_la_SOURCES = \
	color.c \
	swfdec_audio.c \
	swfdec_bits.c \
	swfdec_buffer.c \
	swfdec_button.c \
	swfdec_button_movie.c \
	swfdec_cache.c \
	swfdec_character.c \
	swfdec_codec.c \
	$(CODECS) \
	swfdec_compiler.c \
	swfdec_debug.c \
	swfdec_decoder.c \
	swfdec_edittext.c \
	swfdec_edittext_movie.c \
	swfdec_event.c \
	swfdec_font.c \
	swfdec_graphic.c \
	swfdec_graphic_movie.c \
	swfdec_html_parser.c \
	swfdec_image.c \
	swfdec_js.c \
	swfdec_js_global.c \
	swfdec_js_mouse.c \
	swfdec_js_movie.c \
	swfdec_movie.c \
	swfdec_loader.c \
	swfdec_pattern.c \
	swfdec_player.c \
	swfdec_rect.c \
	swfdec_ringbuffer.c \
	swfdec_root_movie.c \
	swfdec_shape.c \
	swfdec_sound.c \
	swfdec_sprite.c \
	swfdec_sprite_movie.c \
	swfdec_swf_decoder.c \
	swfdec_tag.c \
	swfdec_text.c

libswfdec_@SWFDEC_MAJORMINOR@_la_CFLAGS = \
	$(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS) $(GLIB_CFLAGS) $(PANGO_CFLAGS) \
	-I$(srcdir)/jpeg/ $(js_cflags) $(LIBOIL_CFLAGS) $(FFMPEG_CFLAGS)
libswfdec_@SWFDEC_MAJORMINOR@_la_LDFLAGS = \
	-export-symbols-regex '^(swfdec_.*)' \
	$(CAIRO_LIBS) $(GLIB_LIBS) $(PANGO_LIBS) $(MAD_LIBS) $(FFMPEG_LIBS) \
	$(LIBOIL_LIBS)

libswfdec_@SWFDEC_MAJORMINOR@_la_LIBADD = jpeg/libjpeg.la js/libjs.la

libswfdec_@SWFDEC_MAJORMINOR@includedir = $(includedir)/swfdec-@SWFDEC_MAJORMINOR@

libswfdec_@SWFDEC_MAJORMINOR@include_HEADERS = \
	swfdec.h \
	swfdec_buffer.h \
	swfdec_loader.h \
	swfdec_player.h

noinst_HEADERS = \
	color.h \
	swfdec_audio.h \
	swfdec_bits.h \
	swfdec_button.h \
	swfdec_button_movie.h \
	swfdec_cache.h \
	swfdec_character.h \
	swfdec_codec.h \
	swfdec_compiler.h \
	swfdec_debug.h \
	swfdec_decoder.h \
	swfdec_edittext.h \
	swfdec_edittext_movie.h \
	swfdec_event.h \
	swfdec_font.h \
	swfdec_graphic.h \
	swfdec_graphic_movie.h \
	swfdec_image.h \
	swfdec_js.h \
	swfdec_loader_internal.h \
	swfdec_movie.h \
	swfdec_pattern.h \
	swfdec_player_internal.h \
	swfdec_rect.h \
	swfdec_ringbuffer.h \
	swfdec_root_movie.h \
	swfdec_shape.h \
	swfdec_sound.h \
	swfdec_sprite.h \
	swfdec_sprite_movie.h \
	swfdec_swf_decoder.h \
	swfdec_tag.h \
	swfdec_text.h \
	swfdec_types.h

