SUBDIRS = custom image sound trace various gnash

noinst_PROGRAMS = swfdec-test

swfdec_test_sources = \
	swfdec_test.c \
	swfdec_test_buffer.c \
	swfdec_test_function.c \
	swfdec_test_global.c \
	swfdec_test_image.c \
	swfdec_test_plugin.c \
	swfdec_test_socket.c \
	swfdec_test_swfdec_socket.c \
	swfdec_test_test.c \
	swfdec_test_utils.c

BUILT_SOURCES = \
	swfdec_test_function_list.h

CLEANFILES = \
	$(BUILT_SOURCES)

swfdec_test_SOURCES = \
	$(swfdec_test_sources) \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	swfdec_test_initialize.as

noinst_HEADERS = \
	swfdec_test_buffer.h \
	swfdec_test_function.h \
	swfdec_test_function_list.h \
	swfdec_test_image.h \
	swfdec_test_initialize.h \
	swfdec_test_plugin.h \
	swfdec_test_socket.h \
	swfdec_test_swfdec_socket.h \
	swfdec_test_test.h \
	swfdec_test_utils.h
	
swfdec_test_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
swfdec_test_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS) $(CAIRO_LIBS)

swfdec_test_function_list.h: $(swfdec_test_sources)
	(cd $(srcdir) \
	  && grep -he "^SWFDEC_TEST_FUNCTION" $(swfdec_test_sources) \
	 ) >> xgen-sfl \
	&& (cmp -s xgen-sfl swfdec_test_function_list.h || cp xgen-sfl swfdec_test_function_list.h) \
	&& rm -f xgen-sfl

