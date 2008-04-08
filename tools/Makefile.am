noinst_PROGRAMS = swfdec-extract dump crashfinder

crashfinder_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
crashfinder_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)
crashfinder_SOURCES = crashfinder.c

dump_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS) $(PANGO_CFLAGS)
dump_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS)

swfdec_extract_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
swfdec_extract_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)
