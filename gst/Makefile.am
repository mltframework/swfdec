
plugindir = $(libdir)/gst

plugin_LTLIBRARIES = libgstswfdec.la

libgstswfdec_la_SOURCES = gstswfdec.c
libgstswfdec_la_CFLAGS = $(SWF_CFLAGS) $(GST_CFLAGS) $(LIBART_CFLAGS)
libgstswfdec_la_LDFLAGS = $(SWF_LIBS) $(GST_LIBS) $(LIBART_LIBS) $(GST_PLUGIN_LDFLAGS)

noinst_HEADERS = gstswfdec.h

