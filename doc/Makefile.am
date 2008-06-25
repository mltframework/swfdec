## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=swfdec

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../gtk
DOC_SOURCE_DIR=../swfdec

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=--type-init-func="swfdec_init();"

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED" 
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml --source-dir=../swfdec-gtk

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/gtk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/gtk/*.c
HFILE_GLOB=$(top_srcdir)/swfdec/*.h $(top_srcdir)/swfdec-gtk/*.h
CFILE_GLOB=$(top_srcdir)/swfdec/*.c $(top_srcdir)/swfdec-gtk/*.c

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
IGNORE_HFILES= \
	js \
	jpeg \
	swfdec_access.h \
	swfdec_actor.h \
	swfdec_amf.h \
	swfdec_as_boolean.h \
	swfdec_as_date.h \
	swfdec_as_frame_internal.h \
	swfdec_as_internal.h \
	swfdec_as_interpret.h \
	swfdec_as_math.h \
	swfdec_as_number.h \
	swfdec_as_scope.h \
	swfdec_as_script_function.h \
	swfdec_as_stack.h \
	swfdec_as_string.h \
	swfdec_as_strings.h \
	swfdec_as_super.h \
	swfdec_as_with.h \
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
	swfdec_bits.h \
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
	swfdec_color_as.h \
	swfdec_constant_pool.h \
	swfdec_debug.h \
	swfdec_debugger.h \
	swfdec_decoder.h \
	swfdec_draw.h \
	swfdec_text_field.h \
	swfdec_text_field_movie.h \
	swfdec_enums.h \
	swfdec_event.h \
	swfdec_filter.h \
	swfdec_flv_decoder.h \
	swfdec_function_list.h \
	swfdec_font.h \
	swfdec_gradient_pattern.h \
	swfdec_graphic.h \
	swfdec_graphic_movie.h \
	swfdec_image.h \
	swfdec_image_decoder.h \
	swfdec_internal.h \
	swfdec_interval.h \
	swfdec_js.h \
	swfdec_listener.h \
	swfdec_load_object.h \
	swfdec_load_sound.h \
	swfdec_loader_internal.h \
	swfdec_stream_target.h \
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
	swfdec_resource_request.h \
	swfdec_ringbuffer.h \
	swfdec_root_movie.h \
	swfdec_root_sprite.h \
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
	swfdec_stroke.h \
	swfdec_style_sheet.h \
	swfdec_swf_decoder.h \
	swfdec_tag.h \
	swfdec_text.h \
	swfdec_text_attributes.h \
	swfdec_text_buffer.h \
	swfdec_text_layout.h \
	swfdec_text_format.h \
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
	swfdec_xml.h \
	swfdec_xml_node.h \
	swfdec_xml_socket.h

if WITH_GTK
EXTRA_HFILES = \
	../swfdec-gtk/swfdec_gtk_keys.h \
	../swfdec-gtk/swfdec_gtk_loader.h \
	../swfdec-gtk/swfdec_gtk_player.h \
	../swfdec-gtk/swfdec_gtk_socket.h \
	../swfdec-gtk/swfdec_gtk_widget.h
else
EXTRA_HFILES =
endif
 

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files=

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. INCLUDES=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
INCLUDES=$(SWFDEC_GTK_CFLAGS) $(CAIRO_CFLAGS)
GTKDOC_LIBS=$(SWFDEC_GTK_LIBS) 

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += 

# Comment this out if you want your docs-status tested during 'make check'
#TESTS = $(GTKDOC_CHECK)
