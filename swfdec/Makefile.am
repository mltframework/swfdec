SUBDIRS = jpeg

CODEC_FILES =

if HAVE_GST
CODEC_FILES += swfdec_codec_gst.c swfdec_audio_decoder_gst.c swfdec_video_decoder_gst.c
endif

lib_LTLIBRARIES = libswfdec-@SWFDEC_MAJORMINOR@.la

# these are files that must be in SRCDIR
libswfdec_source_files = \
	swfdec_access.c \
	swfdec_accessibility.c \
	swfdec_actor.c \
	swfdec_amf.c \
	swfdec_as_array.c \
	swfdec_as_boolean.c \
	swfdec_as_context.c \
	swfdec_as_date.c \
	swfdec_as_debugger.c \
	swfdec_as_frame.c \
	swfdec_as_function.c \
	swfdec_as_interpret.c \
	swfdec_as_math.c \
	swfdec_as_native_function.c \
	swfdec_as_number.c \
	swfdec_as_object.c \
	swfdec_as_script_function.c \
	swfdec_as_stack.c \
	swfdec_as_string.c \
	swfdec_as_strings.c \
	swfdec_as_super.c \
	swfdec_as_types.c \
	swfdec_asbroadcaster.c \
	swfdec_audio.c \
	swfdec_audio_decoder.c \
	swfdec_audio_decoder_adpcm.c \
	swfdec_audio_decoder_uncompressed.c \
	$(CODEC_FILES) \
	swfdec_audio_event.c \
	swfdec_audio_load.c \
	swfdec_audio_flv.c \
	swfdec_audio_stream.c \
	swfdec_audio_swf_stream.c \
	swfdec_bevel_filter.c \
	swfdec_bitmap_data.c \
	swfdec_bitmap_filter.c \
	swfdec_bitmap_movie.c \
	swfdec_bitmap_pattern.c \
	swfdec_bits.c \
	swfdec_blur_filter.c \
	swfdec_blur_filter_as.c \
	swfdec_bots.c \
	swfdec_buffer.c \
	swfdec_button.c \
	swfdec_button_movie.c \
	swfdec_button_movie_as.c \
	swfdec_cache.c \
	swfdec_cached.c \
	swfdec_cached_image.c \
	swfdec_cached_video.c \
	swfdec_camera.c \
	swfdec_character.c \
	swfdec_color.c \
	swfdec_color_as.c \
	swfdec_color_matrix_filter.c \
	swfdec_color_matrix_filter_as.c \
	swfdec_color_transform_as.c \
	swfdec_constant_pool.c \
	swfdec_convolution_filter.c \
	swfdec_convolution_matrix.c \
	swfdec_debug.c \
	swfdec_decoder.c \
	swfdec_displacement_map_filter.c \
	swfdec_display_object.c \
	swfdec_display_object_container.c \
	swfdec_draw.c \
	swfdec_drop_shadow_filter.c \
	swfdec_event.c \
	swfdec_event_dispatcher.c \
	swfdec_external_interface.c \
	swfdec_file_loader.c \
	swfdec_file_reference.c \
	swfdec_file_reference_list.c \
	swfdec_filter.c \
	swfdec_filter_as.c \
	swfdec_flv_decoder.c \
	swfdec_font.c \
	swfdec_function_list.c \
	swfdec_gc_object.c \
	swfdec_glow_filter.c \
	swfdec_gradient_filter.c \
	swfdec_gradient_pattern.c \
	swfdec_graphic.c \
	swfdec_graphic_movie.c \
	swfdec_image.c \
	swfdec_image_decoder.c \
	swfdec_interactive_object.c \
	swfdec_init.c \
	swfdec_interval.c \
	swfdec_key_as.c \
	swfdec_load_object.c \
	swfdec_load_object_as.c \
	swfdec_load_sound.c \
	swfdec_loader.c \
	swfdec_loadvars_as.c \
	swfdec_local_connection.c \
	swfdec_microphone.c \
	swfdec_morph_movie.c \
	swfdec_morphshape.c \
	swfdec_mouse_as.c \
	swfdec_movie.c \
	swfdec_movie_as_drawing.c \
	swfdec_movie_asprops.c \
	swfdec_movie_clip_loader.c \
	swfdec_net_connection.c \
	swfdec_net_stream.c \
	swfdec_net_stream_as.c \
	swfdec_path.c \
	swfdec_pattern.c \
	swfdec_player.c \
	swfdec_player_as.c \
	swfdec_player_scripting.c \
	swfdec_print_job.c \
	swfdec_policy_file.c \
	swfdec_rect.c \
	swfdec_rectangle.c \
	swfdec_renderer.c \
	swfdec_resource.c \
	swfdec_ringbuffer.c \
	swfdec_sandbox.c \
	swfdec_script.c \
	swfdec_selection.c \
	swfdec_shape.c \
	swfdec_shape_parser.c \
	swfdec_shared_object.c \
	swfdec_socket.c \
	swfdec_sound.c \
	swfdec_sound_matrix.c \
	swfdec_sound_object.c \
	swfdec_sound_provider.c \
	swfdec_sprite.c \
	swfdec_sprite_movie.c \
	swfdec_sprite_movie_as.c \
	swfdec_stage_as.c \
	swfdec_stream.c \
	swfdec_stream_target.c \
	swfdec_stroke.c \
	swfdec_style_sheet.c \
	swfdec_swf_decoder.c \
	swfdec_system.c \
	swfdec_system_as.c \
	swfdec_system_ime.c \
	swfdec_system_security.c \
	swfdec_tag.c \
	swfdec_text.c \
	swfdec_text_attributes.c \
	swfdec_text_buffer.c \
	swfdec_text_field.c \
	swfdec_text_field_movie.c \
	swfdec_text_field_movie_as.c \
	swfdec_text_field_movie_html.c \
	swfdec_text_format.c \
	swfdec_text_layout.c \
	swfdec_text_renderer.c \
	swfdec_text_snapshot.c \
	swfdec_transform_as.c \
	swfdec_url.c \
	swfdec_utils.c \
	swfdec_version.c \
	swfdec_video.c \
	swfdec_video_decoder.c \
	swfdec_video_decoder_screen.c \
	swfdec_video_decoder_vp6_alpha.c \
	swfdec_video_movie.c \
	swfdec_video_movie_as.c \
	swfdec_video_provider.c \
	swfdec_video_video_provider.c \
	swfdec_xml_node.c \
	swfdec_xml.c \
	swfdec_xml_socket.c

libswfdec_@SWFDEC_MAJORMINOR@_la_SOURCES = \
  	$(libswfdec_source_files) \
	swfdec_asnative.c \
	swfdec_enums.c \
	swfdec_marshal.c

libswfdec_@SWFDEC_MAJORMINOR@_la_CFLAGS = \
	$(GLOBAL_CFLAGS) $(CAIRO_CFLAGS) $(GLIB_CFLAGS) $(PANGO_CFLAGS) \
	-I$(top_srcdir) -I$(srcdir)/jpeg/ $(LIBOIL_CFLAGS) \
	$(GST_CFLAGS) $(FFMPEG_CFLAGS) $(MAD_CFLAGS) \
	-DG_LOG_DOMAIN=\"Swfdec\"
libswfdec_@SWFDEC_MAJORMINOR@_la_LDFLAGS = \
	$(SYMBOLIC_LDFLAGS) \
	-version-info $(SWFDEC_LIBVERSION) \
	-export-symbols-regex '^(swfdec_.*)' \
	$(CAIRO_LIBS) $(GLIB_LIBS) $(PANGO_LIBS) $(LIBOIL_LIBS) -lz \
	$(MAD_LIBS) $(FFMPEG_LIBS) $(GST_LIBS)

public_headers = \
	swfdec.h \
	swfdec_as_array.h \
	swfdec_as_context.h \
	swfdec_as_debugger.h \
	swfdec_as_frame.h \
	swfdec_as_function.h \
	swfdec_as_native_function.h \
	swfdec_as_object.h \
	swfdec_as_types.h \
	swfdec_audio.h \
	swfdec_buffer.h \
	swfdec_file_loader.h \
	swfdec_gc_object.h \
	swfdec_keys.h \
	swfdec_loader.h \
	swfdec_player.h \
	swfdec_player_scripting.h \
	swfdec_rectangle.h \
	swfdec_renderer.h \
	swfdec_script.h \
	swfdec_socket.h \
	swfdec_stream.h \
	swfdec_system.h \
	swfdec_url.h

libswfdec_@SWFDEC_MAJORMINOR@_la_LIBADD = jpeg/libjpeg.la 

libswfdec_@SWFDEC_MAJORMINOR@includedir = $(includedir)/swfdec-@SWFDEC_MAJORMINOR@/swfdec
libswfdec_@SWFDEC_MAJORMINOR@include_HEADERS = $(public_headers) swfdec_enums.h
nodist_libswfdec_@SWFDEC_MAJORMINOR@include_HEADERS = swfdec_version.h

noinst_HEADERS = \
	swfdec_access.h \
	swfdec_actor.h \
	swfdec_amf.h \
	swfdec_as_boolean.h \
	swfdec_as_frame_internal.h \
	swfdec_as_initialize.h \
	swfdec_as_internal.h \
	swfdec_as_date.h \
	swfdec_as_interpret.h \
	swfdec_as_number.h \
	swfdec_as_script_function.h \
	swfdec_as_stack.h \
	swfdec_as_string.h \
	swfdec_as_strings.h \
	swfdec_as_super.h \
	swfdec_asnative.h \
	swfdec_audio_internal.h \
	swfdec_audio_decoder.h \
	swfdec_audio_decoder_adpcm.h \
	swfdec_audio_decoder_gst.h \
	swfdec_audio_decoder_uncompressed.h \
	swfdec_audio_event.h \
	swfdec_audio_flv.h \
	swfdec_audio_load.h \
	swfdec_audio_stream.h \
	swfdec_audio_swf_stream.h \
	swfdec_bitmap_data.h \
	swfdec_bitmap_movie.h \
	swfdec_bitmap_pattern.h \
	swfdec_bits.h \
	swfdec_blur_filter.h \
	swfdec_bots.h \
	swfdec_button.h \
	swfdec_button_movie.h \
	swfdec_cache.h \
	swfdec_cached.h \
	swfdec_cached_image.h \
	swfdec_cached_video.h \
	swfdec_character.h \
	swfdec_codec_gst.h \
	swfdec_color.h \
	swfdec_color_transform_as.h \
	swfdec_color_matrix_filter.h \
	swfdec_constant_pool.h \
	swfdec_convolution_matrix.h \
	swfdec_debug.h \
	swfdec_decoder.h \
	swfdec_display_object.h \
	swfdec_display_object_container.h \
	swfdec_draw.h \
	swfdec_text_field.h \
	swfdec_text_field_movie.h \
	swfdec_event.h \
	swfdec_event_dispatcher.h \
	swfdec_filter.h \
	swfdec_flv_decoder.h \
	swfdec_font.h \
	swfdec_function_list.h \
	swfdec_gradient_pattern.h \
	swfdec_graphic.h \
	swfdec_graphic_movie.h \
	swfdec_image.h \
	swfdec_image_decoder.h \
	swfdec_initialize.h \
	swfdec_interactive_object.h \
	swfdec_internal.h \
	swfdec_interval.h \
	swfdec_load_object.h \
	swfdec_load_sound.h \
	swfdec_loader_internal.h \
	swfdec_marshal.h \
	swfdec_morph_movie.h \
	swfdec_morphshape.h \
	swfdec_movie.h \
	swfdec_movie_clip_loader.h \
	swfdec_net_connection.h \
	swfdec_net_stream.h \
	swfdec_path.h \
	swfdec_pattern.h \
	swfdec_player_internal.h \
	swfdec_policy_file.h \
	swfdec_rect.h \
	swfdec_renderer_internal.h \
	swfdec_resource.h \
	swfdec_ringbuffer.h \
	swfdec_sandbox.h \
	swfdec_script_internal.h \
	swfdec_shape.h \
	swfdec_shape_parser.h \
	swfdec_sound.h \
	swfdec_sound_matrix.h \
	swfdec_sound_object.h \
	swfdec_sound_provider.h \
	swfdec_sprite.h \
	swfdec_sprite_movie.h \
	swfdec_swf_decoder.h \
	swfdec_stream_target.h \
	swfdec_stroke.h \
	swfdec_style_sheet.h \
	swfdec_tag.h \
	swfdec_text.h \
	swfdec_text_attributes.h \
	swfdec_text_buffer.h \
	swfdec_text_format.h \
	swfdec_text_layout.h \
	swfdec_transform_as.h \
	swfdec_types.h \
	swfdec_utils.h \
	swfdec_video.h \
	swfdec_video_decoder.h \
	swfdec_video_decoder_gst.h \
	swfdec_video_decoder_screen.h \
	swfdec_video_decoder_vp6_alpha.h \
	swfdec_video_movie.h \
	swfdec_video_provider.h \
	swfdec_video_video_provider.h \
	swfdec_xml_node.h \
	swfdec_xml.h \
	swfdec_xml_socket.h

EXTRA_DIST = \
	compiler.c \
	swfdec_as_initialize.as \
	swfdec_initialize.as \
	swfdec_marshal.list \
	swfdec_version.h.in

BUILT_SOURCES = \
	swfdec_asnative.c \
	swfdec_as_strings.h \
	swfdec_enums.c \
	swfdec_enums.h \
	swfdec_marshal.c \
	swfdec_marshal.h

CLEANFILES = \
	$(BUILT_SOURCES)

DISTCLEANFILES = swfdec_version.h

swfdec_asnative.c: $(libswfdec_source_files)
	(cd $(srcdir) \
	  && echo "#include \"swfdec_as_types.h\"" \
	  && echo "#define SWFDEC_AS_NATIVE(x,y,func) void func (SwfdecAsContext *cx, \\" \
	  && echo "    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret);" \
	  && echo "#define SWFDEC_AS_CONSTRUCTOR(x,y,func,type) SWFDEC_AS_NATIVE(x,y,func) GType type (void);" \
	  && grep -he "^SWFDEC_AS_CONSTRUCTOR" $(libswfdec_source_files) \
	  && grep -he "^SWFDEC_AS_NATIVE" $(libswfdec_source_files) \
	  && echo "#undef SWFDEC_AS_CONSTRUCTOR" \
	  && echo "#undef SWFDEC_AS_NATIVE" \
	  && echo "#define SWFDEC_AS_NATIVE(x,y,func) SWFDEC_AS_CONSTRUCTOR(x,y,func,NULL)" \
	  && echo "#define SWFDEC_AS_CONSTRUCTOR(x,y,func,type) { x, y, func, G_STRINGIFY (func), type }," \
	  && echo "const struct { guint x, y; SwfdecAsNative func; const char *name; GType (* get_type) (void); } native_funcs[] = {" \
	  && grep -he "^SWFDEC_AS_CONSTRUCTOR" $(libswfdec_source_files) \
	  && grep -he "^SWFDEC_AS_NATIVE" $(libswfdec_source_files) \
	  && echo "  { 0, 0, NULL }" \
	  && echo "};" \
	  && echo "#undef SWFDEC_AS_CONSTRUCTOR" \
	  && echo "#undef SWFDEC_AS_NATIVE" \
	 ) >> xgen-san \
	&& (cmp -s xgen-san swfdec_asnative.c || cp xgen-san swfdec_asnative.c) \
	&& rm -f xgen-san

swfdec_marshal.h: swfdec_marshal.list Makefile
	$(GLIB_GENMARSHAL) --prefix=swfdec_marshal $(srcdir)/swfdec_marshal.list --header >> xgen-smh \
	&& (cmp -s xgen-smh swfdec_marshal.h || cp xgen-smh swfdec_marshal.h) \
	&& rm -f xgen-smh

swfdec_marshal.c: swfdec_marshal.list swfdec_marshal.h Makefile
	(echo "#include \"swfdec_marshal.h\""; \
	 $(GLIB_GENMARSHAL) --prefix=swfdec_marshal $(srcdir)/swfdec_marshal.list --body;) >> xgen-smc \
	&& cp xgen-smc swfdec_marshal.c \
	&& rm -f xgen-smc 

swfdec_enums.h: $(public_headers) Makefile
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __SWFDEC_ENUMS_H__\n#define __SWFDEC_ENUMS_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define SWFDEC_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __SWFDEC_ENUMS_H__ */" \
		$(public_headers) ) >> xgen-seh \
	&& (cmp -s xgen-seh swfdec_enums.h || cp xgen-seh swfdec_enums.h ) \
	&& rm -f xgen-seh

swfdec_enums.c: $(public_headers) Makefile
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include \"swfdec.h\"\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (g_intern_static_string (\"@EnumName@\"), values);\n  }\n  return etype;\n}\n" \
		$(public_headers) ) > xgen-sec \
	&& (cmp -s xgen-sec swfdec_enums.c || cp xgen-sec swfdec_enums.c ) \
	&& rm -f xgen-sec

swfdec_as_strings.h: swfdec_as_strings.c
	( cd $(srcdir) && echo "/* This is a generated file. See swfdec_as_strings.c for details. */" \
	  && echo "#ifndef _SWFDEC_AS_STRINGS_H_" \
	  && echo "#define _SWFDEC_AS_STRINGS_H_" \
	  && echo "G_BEGIN_DECLS" \
	  && echo "extern const char swfdec_as_strings[];" \
	  && grep "  SWFDEC_AS_CONSTANT_STRING" swfdec_as_strings.c \
	  | sed "s/.*(\"\(.*\)\").*/\1/" \
	  | LC_ALL="C" $(AWK) '{ if ($$0 == "") name = "EMPTY"; else if ($$0 == ",") name = "COMMA"; else if ($$0 == "/") name = "SLASH"; else name = $$0; gsub("[^a-zA-Z0-9]","_",name); print "#define SWFDEC_AS_STR_" name " &swfdec_as_strings[" x + 1 "]"; x = x + length ($$0) + 2 }' \
	  && echo "G_END_DECLS" \
	  && echo "#endif" ) > xgen-sas \
	&& (cmp -s xgen-sas swfdec_as_strings.h || cp xgen-sas swfdec_as_strings.h ) \
	&& rm -f xgen-sas
