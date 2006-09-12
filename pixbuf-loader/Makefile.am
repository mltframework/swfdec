loaderdir = $(libdir)/gtk-2.0/$(GTK_VERSION)/loaders

INCLUDES = $(GLOBAL_CFLAGS) \
	$(SWF_CFLAGS) $(CAIRO_CFLAGS) \
	$(PIXBUF_CFLAGS) \
	-DGDK_PIXBUF_ENABLE_BACKEND \
	-I$(top_srcdir)

if ENABLE_PIXBUF_LOADER
target_pixbuf_loader = swf_loader.la 
else
target_pixbuf_loader = 
endif

swf_loader_la_SOURCES = io-swf.c io-swf-anim.c
swf_loader_la_LDFLAGS = -avoid-version -module
swf_loader_la_LIBADD = $(PIXBUF_LIBS)\
	$(SWF_LIBS) $(CAIRO_LIBS)

noinst_HEADERS = io-swf-anim.h

loader_LTLIBRARIES = $(target_pixbuf_loader)

install-data-local: 
	if [ -z "$(DESTDIR)" ] ; then \
	  $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/gtk-2.0 ; \
	  $(QUERYLOADERS) > $(DESTDIR)$(sysconfdir)/gtk-2.0/gdk-pixbuf.loaders ; \
	fi
