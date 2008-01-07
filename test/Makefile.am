SUBDIRS = image sound test trace various


if WITH_GTK
noinst_LTLIBRARIES = libswfedit.la
noinst_PROGRAMS = swfdec-extract dump swfedit swfscript crashfinder
else
noinst_PROGRAMS = swfdec-extract dump crashfinder
endif

crashfinder_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
crashfinder_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)
crashfinder_SOURCES = crashfinder.c

dump_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS) $(PANGO_CFLAGS)
dump_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS) $(PANGO_LIBS)

swfdec_extract_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
swfdec_extract_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

libswfedit_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS)
libswfedit_la_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS)

swfedit_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS)
swfedit_LDFLAGS = $(SWFDEC_LIBS) $(GTK_LIBS)
swfedit_LDADD = libswfedit.la

swfscript_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(GTK_CFLAGS)
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
