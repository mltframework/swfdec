check_PROGRAMS = test-scripts
TESTS = $(check_PROGRAMS)

test_scripts_SOURCES = test-scripts.c

EXTRA_DIST = funcall.as

test_scripts_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS) \
	-I$(top_srcdir)/libswfdec/js/ -I$(top_builddir)/libswfdec/js -DXP_UNIX
test_scripts_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS) $(top_builddir)/libswfdec/js/libjs.la


