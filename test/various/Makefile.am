check_PROGRAMS = ringbuffer urlencode
TESTS = $(check_PROGRAMS)

ringbuffer_SOURCES = ringbuffer.c
ringbuffer_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
ringbuffer_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

urlencode_SOURCES = urlencode.c
urlencode_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
urlencode_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

