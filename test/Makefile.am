SUBDIRS = sound trace various

noinst_PROGRAMS = swfdec-extract dump parse

dump_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS) $(PANGO_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
dump_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS)

parse_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS)
parse_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

swfdec_extract_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfdec_extract_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

