
plugindir = $(libdir)/mozilla/plugins/

plugin_LTLIBRARIES = libmozswfdec.la

libmozswfdec_la_SOURCES = plugin.c npunix.c
libmozswfdec_la_CFLAGS = $(MOZILLA_CFLAGS) $(X_CFLAGS)
libmozswfdec_la_LDFLAGS = $(X_LIBS)
libmozswfdec_la_LIBADD = -lXt

