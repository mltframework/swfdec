noinst_PROGRAMS = rtmp-server

TESTS = $(check_PROGRAMS)

rtmp_server_SOURCES = rtmp_server.c
rtmp_server_CFLAGS = $(GLOBAL_CFLAGS) $(GTK_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
rtmp_server_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS) $(CAIRO_LIBS)

