SUBDIRS = image sound trace various

noinst_LTLIBRARIES = libswfedit.la

if WITH_GTK
noinst_PROGRAMS = swfdec-extract dump parse swfedit swfscript
else
noinst_PROGRAMS = swfdec-extract dump parse 
endif

dump_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS) $(PANGO_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
dump_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS)

parse_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
parse_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

swfdec_extract_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfdec_extract_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

libswfedit_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
libswfedit_la_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS)

swfedit_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfedit_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS)
swfedit_LDADD = libswfedit.la

swfscript_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS) -DXP_UNIX -I$(top_builddir)/libswfdec/js
swfscript_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS)
swfscript_LDADD = libswfedit.la

libswfedit_la_SOURCES = \
	swfdec_out.c \
	swfedit_file.c \
	swfedit_list.c \
	swfedit_tag.c \
	swfedit_token.c

noinst_HEADERS = \
	swfdec_out.h \
	swfedit_file.h \
	swfedit_list.h \
	swfedit_tag.h \
	swfedit_token.h
