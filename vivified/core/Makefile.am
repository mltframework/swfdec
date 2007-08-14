noinst_LTLIBRARIES = libvivified-core.la

libvivified_core_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_GTK_CFLAGS) $(VIVI_CFLAGS)
libvivified_core_la_LDFLAGS = $(SWFDEC_GTK_LIBS) $(VIVI_LIBS)

libvivified_core_la_SOURCES = \
	vivi_application.c \
	vivi_application_as.c \
	vivi_application.c \
	vivi_breakpoint.c \
	vivi_debugger.c \
	vivi_function.c \
	vivi_initialize.as \
	vivi_initialize.s \
	vivi_ming.c

vivi_initialize.lo: vivi_initialize.s vivi_initialize.as
	$(LTCCASCOMPILE) -c -o $@ vivi_initialize.s

noinst_HEADERS = \
	vivi_application.h \
	vivi_breakpoint.h \
	vivi_debugger.h \
	vivi_function.h \
	vivi_function_list.h \
	vivi_ming.h \
	vivified-core.h

BUILT_SOURCES = \
	vivi_function_list.h

CLEANFILES = \
	$(BUILT_SOURCES)

vivi_function_list.h: $(libvivified_core_la_SOURCES)
	(cd $(srcdir) \
	  && grep -he "^VIVI_FUNCTION" $(libvivified_core_la_SOURCES) \
	 ) >> xgen-vfl \
	&& (cmp -s xgen-vfl swfdec_asnative.h || cp xgen-vfl vivi_function_list.h) \
	&& rm -f xgen-vfl
