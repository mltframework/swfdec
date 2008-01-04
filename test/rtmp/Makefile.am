noinst_PROGRAMS = rtmp-server

check_PROGRAMS = sound
TESTS = $(check_PROGRAMS)

rtmp_server_SOURCES = rtmp_server.c
rtmp_server_CFLAGS = $(GLOBAL_CFLAGS) $(HTTP_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
rtmp_server_LDFLAGS = $(SWFDEC_LIBS) $(HTTP_LIBS) $(CAIRO_LIBS)

