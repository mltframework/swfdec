check_PROGRAMS = gc ringbuffer urlencode
TESTS = $(check_PROGRAMS)

gc_SOURCES = gc.c
gc_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
gc_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

ringbuffer_SOURCES = ringbuffer.c
ringbuffer_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
ringbuffer_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

urlencode_SOURCES = urlencode.c
urlencode_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
urlencode_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

