SUBDIRS = actionscript

noinst_PROGRAMS = test-pixbuf dump parse

test_pixbuf_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(CAIRO_CFLAGS)
test_pixbuf_LDFLAGS = $(GTK_LIBS) $(CAIRO_LIBS)

dump_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS) \
	-I$(top_srcdir)/libswfdec/js/ -I$(top_builddir)/libswfdec/js -DXP_UNIX
dump_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

parse_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS)
parse_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

