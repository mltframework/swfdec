noinst_LTLIBRARIES = libvivified-core.la

libvivified_core_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_GTK_CFLAGS) $(VIVI_CFLAGS)
libvivified_core_la_LDFLAGS = $(SWFDEC_GTK_LIBS) $(VIVI_LIBS)

libvivified_core_la_SOURCES = \
	$(libvivified_core_sources) \
	vivi_marshal.c

libvivified_core_sources = \
	vivi_application.c \
	vivi_application_as.c \
	vivi_application.c \
	vivi_breakpoint.c \
	vivi_debugger.c \
	vivi_function.c \
	vivi_initialize.as \
	vivi_initialize.s \
	vivi_ming.c \
	vivi_player_as.c \
	vivi_wrap.c \
	vivi_wrap_as.c

vivi_initialize.lo: vivi_initialize.s vivi_initialize.as
	$(LTCCASCOMPILE) -c -o $@ vivi_initialize.s

noinst_HEADERS = \
	vivi_application.h \
	vivi_breakpoint.h \
	vivi_debugger.h \
	vivi_function.h \
	vivi_function_list.h \
	vivi_marshal.h \
	vivi_ming.h \
	vivi_wrap.h \
	vivified-core.h

BUILT_SOURCES = \
	vivi_function_list.h \
	vivi_marshal.h \
	vivi_marshal.c

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	vivi_marshal.list

vivi_function_list.h: $(libvivified_core_source)
	(cd $(srcdir) \
	  && grep -he "^VIVI_FUNCTION" $(libvivified_core_sources) \
	 ) >> xgen-vfl \
	&& (cmp -s xgen-vfl vivi_function_list.h || cp xgen-vfl vivi_function_list.h) \
	&& rm -f xgen-vfl

vivi_marshal.h: vivi_marshal.list Makefile
	$(GLIB_GENMARSHAL) --prefix=vivi_marshal $(srcdir)/vivi_marshal.list --header >> xgen-vmh \
	&& (cmp -s xgen-vmh vivi_marshal.h || cp xgen-vmh vivi_marshal.h) \
	&& rm -f xgen-vmh

vivi_marshal.c: vivi_marshal.list vivi_marshal.h Makefile
	(echo "#include \"vivi_marshal.h\""; \
	 $(GLIB_GENMARSHAL) --prefix=vivi_marshal $(srcdir)/vivi_marshal.list --body;) >> xgen-vmc \
	&& cp xgen-vmc vivi_marshal.c \
	&& rm -f xgen-vmc 

