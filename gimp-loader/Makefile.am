if HAVE_GIMP
gimpdir = $(gimplibdir)/plug-ins
else
gimpdir = 
endif

INCLUDES = $(SWF_CFLAGS)\
	$(LIBGIMP_CFLAGS)\
	-I$(top_srcdir)

if HAVE_GIMP
gimp_PROGRAMS = swf
else
gimp_PROGRAMS =
endif

swf_SOURCES=swf-gimp.c
swf_LDFLAGS = -no-undefined -avoid-version -module
swf_LDADD = $(LIBGIMP_LIBS) $(SWF_LIBS)

