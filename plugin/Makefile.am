
plugindir = $(libdir)/mozilla/plugins/

plugin_LTLIBRARIES = libswfdecmozilla.la

libswfdecmozilla_la_SOURCES = plugin.c
libswfdecmozilla_la_CFLAGS = $(GLOBAL_CFLAGS) $(MOZILLA_CFLAGS) $(X_CFLAGS) \
	-DBINDIR=\"$(bindir)/\"
libswfdecmozilla_la_LDFLAGS = -module -avoid-version $(X_LIBS)
libswfdecmozilla_la_LIBADD = -lXt

bin_PROGRAMS = swfdec-mozilla-player

swfdec_mozilla_player_SOURCES = \
	swfdec-mozilla-player.c \
	gstappsrc.c \
	gstappsrc.h \
	spp.h \
	gstswfdec.c \
	gstswfdec.h

swfdec_mozilla_player_CFLAGS = \
	$(GLOBAL_CFLAGS) \
	$(SWF_CFLAGS) \
	$(GST_CFLAGS) \
	$(GTK_CFLAGS)

swfdec_mozilla_player_LDADD = \
	$(GST_LIBS) \
	$(GTK_LIBS) \
	$(SWF_LIBS)

