
plugindir = $(libdir)/mozilla/plugins/

plugin_LTLIBRARIES = libmozswfdec.la

libmozswfdec_la_SOURCES = plugin.c npunix.c
libmozswfdec_la_CFLAGS = \
	-I/usr/include/mozilla-1.0.0/plugin \
	-I/usr/include/mozilla-1.0.0/java \
	-I/usr/include/mozilla-1.0.0/nspr \
	$(GLIB12_CFLAGS) $(GDK_CFLAGS)
libmozswfdec_la_LDFLAGS = -L/usr/X11R6/lib -lXt $(MOZ_PLUGIN_LDFLAGS)

