noinst_LTLIBRARIES = libvivified-compiler.la

libvivified_compiler_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
libvivified_compiler_la_LDFLAGS = $(SWFDEC_LIBS)

libvivified_compiler_la_SOURCES = \
	vivi_decompiler.c

noinst_HEADERS = \
	vivi_decompiler.h \
	vivified-compiler.h


noinst_PROGRAMS = vivi-decompile

vivi_decompile_SOURCES = \
	decompiler.c

vivi_decompile_CFLAGS =  $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
vivi_decompile_LDFLAGS = $(SWFDEC_LIBS)
vivi_decompile_LDADD = libvivified-compiler.la
