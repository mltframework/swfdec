
plugindir = $(libdir)/mozilla/plugins/

plugin_LTLIBRARIES = libmozswfdec.la

libmozswfdec_la_SOURCES = plugin.c
libmozswfdec_la_CFLAGS = $(MOZILLA_CFLAGS) $(X_CFLAGS) -DBINDIR=\"$(bindir)\"
libmozswfdec_la_LDFLAGS = -module -avoid-version $(X_LIBS)
libmozswfdec_la_LIBADD = -lXt

