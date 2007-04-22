AC_PREREQ([2.58])
AC_INIT(swfdec,0.4.3.1)

[nano=$(echo $PACKAGE_VERSION | sed 's/[0-9]\.[0-9]\.[0-9][0-9]*\.*//')]
if test x"$nano" = x1 ; then
  SWFDEC_CVS="yes"
else
  SWFDEC_CVS="no"
fi

AM_INIT_AUTOMAKE(1.6)
dnl AC_CANONICAL_TARGET([])
AM_MAINTAINER_MODE

SWFDEC_MAJORMINOR=0.4
AC_SUBST(SWFDEC_MAJORMINOR)

AM_CONFIG_HEADER(config.h)
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl decide on error flags
dnl if we support them, we set them unconditionally
AS_COMPILER_FLAG(-Wall, GLOBAL_CFLAGS="-Wall", GLOBAL_CFLAGS="")
dnl I want this but stupid headers don't let me
dnl AS_COMPILER_FLAG(-Wshadow, GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Wshadow")
AS_COMPILER_FLAG(-Wextra -Wno-missing-field-initializers -Wno-unused-parameter, GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
dnl if we're in nano >= 1, add -Werror if supported
if test x$SWFDEC_CVS = xyes ; then
  AS_COMPILER_FLAG(-Werror, GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Werror")
  DEFAULT_DEBUG_LEVEL="SWFDEC_LEVEL_WARNING"
else
  DEFAULT_DEBUG_LEVEL="SWFDEC_LEVEL_ERROR"
fi
AC_DEFINE_UNQUOTED(SWFDEC_LEVEL_DEFAULT, $DEFAULT_DEBUG_LEVEL, [Default debug level used])

SWFDEC_LIBVERSION="3:0:0"
AC_SUBST(SWFDEC_LIBVERSION)
AM_PROG_LIBTOOL

dnl Add parameters for aclocal
dnl (This must come after AM_INIT_AUTOMAKE, since it modifies ACLOCAL)
#ACLOCAL_AMFLAGS="-I m4 $ACLOCAL_AMFLAGS"

AC_PROG_CC
AM_PROG_CC_STDC

AC_HEADER_STDC([])

dnl ##############################
dnl # Do automated configuration #
dnl ##############################

dnl Check for tools:
dnl ================

dnl modify pkg-config path
AC_ARG_WITH(pkg-config-path, 
   AC_HELP_STRING([--with-pkg-config-path],[colon-separated list of pkg-config(1) dirs]),
   [export PKG_CONFIG_PATH=${withval}])

dnl Check for essential libraries first:
dnl ====================================

GLIB_VER=2.0
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_VER gobject-2.0 >= $GLIB_VER,
		HAVE_GLIB=yes, HAVE_GLIB=no)
if test "$HAVE_GLIB" = "no"; then
  AC_MSG_ERROR([glib-2.0 >= $GLIB_VER and gobject-2.0 >= $GLIB_VER are required to build swfdec])
fi
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_VER)
dnl FIXME: detect these executables correctly
GLIB_GENMARSHAL=glib-genmarshal
AC_SUBST(GLIB_GENMARSHAL)
GLIB_MKENUMS=glib-mkenums
AC_SUBST(GLIB_MKENUMS)

PKG_CHECK_MODULES(PANGO, pangocairo, HAVE_PANGO=yes, HAVE_PANGO=no)
if test "$HAVE_PANGO" = "no"; then
  AC_MSG_ERROR([pangocairo is required to build swfdec])
fi
AC_SUBST(PANGO_LIBS)
AC_SUBST(PANGO_CFLAGS)

AC_ARG_ENABLE(gtk,
	AS_HELP_STRING([--enable-gtk],
			[enable Gtk integration (default=yes)])],
	enable_gtk=$enableval,
	enable_gtk="yes")

dnl 
dnl GTK: We want this for swfdec-gtk
dnl
GTK_VER=2.8.0
AC_SUBST(GTK_VER)
if test "$enable_gtk" = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_VER, HAVE_GTK=yes, HAVE_GTK=no)
	if test "$HAVE_GTK" = "no"; then
	  AC_MSG_WARN([cannot find GTK+-2.0, player will be disabled])
	fi
	if test "x$HAVE_GTK" = xyes; then
	  AC_DEFINE(HAVE_GTK, 1, [Define if Gtk is enabled])
	else
	  AC_MSG_ERROR([Couldn't find a suitable Gtk version. You need at least version $GTK_VER])
	fi
else
	AC_MSG_WARN([*** Gtk support was not enabled. ***])
fi
AM_CONDITIONAL(WITH_GTK, [test "x$HAVE_GTK" = xyes])

dnl
dnl audio backend
dnl
AC_ARG_WITH(audio,
            [AC_HELP_STRING([--with-audio=@<:@auto/alsa/none@:>@],
                            [audio backend to use])],,
	    [with_audio=auto])

AUDIO_TYPE=
if test "$with_audio" = "auto" -o "$with_audio" = "alsa"; then
  PKG_CHECK_MODULES(ALSA, alsa >= 1.0, AUDIO_TYPE=alsa)
  if test "$AUDIO_TYPE" = "alsa"; then
    with_audio=alsa
  else
    if test "$with_audio" = "alsa"; then
      AC_MSG_ERROR([no alsa audio support])
    else
      AC_MSG_WARN([no alsa audio support])
    fi
  fi
  AUDIO_CFLAGS=$ALSA_CFLAGS
  AUDIO_LIBS=$ALSA_LIBS
fi

if test "$with_audio" = "auto" -o "$with_audio" = "none"; then
  AUDIO_CFLAGS=
  AUDIO_LIBS=
  AUDIO_TYPE=none
fi

if test "x$AUDIO_TYPE" = "x"; then
  AC_MSG_ERROR([desired audio support could not be used])
else
  AC_MSG_NOTICE([audio backend: $AUDIO_TYPE])
fi
AC_SUBST(AUDIO_LIBS)
AC_SUBST(AUDIO_CFLAGS)
AC_SUBST(AUDIO_TYPE)

LIBOIL_VER=0.3.1
PKG_CHECK_MODULES(LIBOIL, liboil-0.3 >= $LIBOIL_VER, HAVE_LIBOIL=yes, HAVE_LIBOIL=no)
AC_SUBST(LIBOIL_LIBS)
AC_SUBST(LIBOIL_CFLAGS)
if test "$HAVE_LIBOIL" = "no"; then
  AC_MSG_ERROR([liboil-0.3 >= $LIBOIL_VER is required to build swfdec])
fi

CAIRO_VER=1.2.0
PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_VER, HAVE_CAIRO=yes, HAVE_CAIRO=no)
AC_SUBST(CAIRO_LIBS)
AC_SUBST(CAIRO_CFLAGS)
if test "$HAVE_CAIRO" = "no"; then
  AC_MSG_ERROR([cairo >= $CAIRO_VER is required to build swfdec])
fi

AC_ARG_ENABLE(mad,
	AS_HELP_STRING([--enable-mad],
			[enable mad audio (default=yes)])],
	enable_mad=$enableval,
	enable_mad="yes")

if test "$enable_mad" = "yes"; then
	AC_CHECK_LIB(mad, mad_decoder_finish, HAVE_MAD="yes" MAD_LIBS="-lmad", HAVE_MAD="no")
	AC_SUBST(MAD_LIBS)
	if test "x$HAVE_MAD" = xyes; then
	  AC_DEFINE(HAVE_MAD, 1, [Define if mad is enabled])
	else
	  AC_MSG_ERROR([Couldn't find mad. You might need to install the libmad0-dev package.])
	fi
else
	AC_MSG_WARN([*** mad audio support was not enabled. ***])
fi
AM_CONDITIONAL(HAVE_MAD, [test "x$HAVE_MAD" = xyes])

dnl  I'd put a required version in here if distros can agree on a way
dnl  to detect ffmpeg. But as it stands even pkg-config versions are weird.
dnl  So you'll have to update your ffmpeg checkout if compilation fails.
dnl  Or you submit a patch that detects ffmpeg reliably on the distros.
AC_ARG_ENABLE(ffmpeg,
	AS_HELP_STRING([--enable-ffmpeg],
			[enable ffmpeg support (default=yes)])],
	enable_ffmpeg=$enableval,
	enable_ffmpeg="yes")

if test "$enable_ffmpeg" = "yes"; then
	PKG_CHECK_MODULES(FFMPEG, libavcodec, HAVE_FFMPEG=yes, HAVE_FFMPEG=no)
	AC_SUBST(FFMPEG_CFLAGS)
	AC_SUBST(FFMPEG_LIBS)
	if test "x$HAVE_FFMPEG" = xyes; then
	  AC_DEFINE(HAVE_FFMPEG, 1, [Define if ffmpeg is enabled])
	else
	  AC_MSG_ERROR([Couldn't find ffmpeg. You might need to install the libavcodec-dev package.])
	fi
else
	AC_MSG_WARN([*** ffmpeg support was not enabled. ***])
fi
AM_CONDITIONAL(HAVE_FFMPEG, [test "x$HAVE_FFMPEG" = xyes])
AC_ARG_ENABLE(gstreamer,
	AS_HELP_STRING([--enable-gstreamer],
			[enable GStreamer support (default=yes)])],
	enable_gstreamer=$enableval,
	enable_gstreamer="yes")

if test "$enable_gstreamer" = "yes"; then
	GST_REQUIRED=0.10.11
	PKG_CHECK_MODULES(GST, gstreamer-0.10 >= $GST_REQUIRED, HAVE_GST=yes, HAVE_GST=no)
	if test "x$HAVE_GST" = xyes; then
	  AC_DEFINE(HAVE_GST, 1, [Define if GStreamer is enabled])
	else
	  AC_MSG_ERROR([Couldn't find GStreamer $GST_REQUIRED.])
	fi
else
	AC_MSG_WARN([*** GStreamer support was not enabled. ***])
fi
AM_CONDITIONAL(HAVE_GST, [test "x$HAVE_GST" = xyes])


AC_ARG_ENABLE(gnome-vfs,
	AS_HELP_STRING([--enable-gnome-vfs],
			[enable gnome-vfs support for swfdec-gtk (default=yes)])],
	enable_gnomevfs=$enableval,
	enable_gnomevfs="yes")

if test "$enable_gnomevfs" = "yes"; then
	PKG_CHECK_MODULES(GNOMEVFS, gnome-vfs-2.0 >= 2.14.0, HAVE_GNOMEVFS=yes, HAVE_GNOMEVFS=no)
	if test "x$HAVE_GNOMEVFS" = xyes; then
	  AC_DEFINE(HAVE_GNOMEVFS, 1, [Define if gnome-vfs is enabled])
	else
	  AC_MSG_ERROR([Couldn't find gnome-vfs-2.0.])
	fi
else
	AC_MSG_WARN([*** gnome-vfs support was not enabled. ***])
fi
AM_CONDITIONAL(HAVE_GNOMEVFS, [test "x$HAVE_GNOMEVFS" = xyes])

AC_SUBST(GLOBAL_CFLAGS)
AC_SUBST(GLOBAL_CFLAGS)

SWFDEC_CFLAGS="-I\$(top_srcdir) $GLIB_CFLAGS $CAIRO_CFLAGS"
SWFDEC_LIBS="\$(top_builddir)/libswfdec/libswfdec-$SWFDEC_MAJORMINOR.la $GLIB_LIBS $CAIRO_LIBS -lz"
AC_SUBST(SWFDEC_LIBS)
AC_SUBST(SWFDEC_CFLAGS)

SWFDEC_GTK_CFLAGS="$SWFDEC_CFLAGS $GTK_CFLAGS"
SWFDEC_GTK_LIBS="\$(top_builddir)/libswfdec-gtk/libswfdec-gtk-$SWFDEC_MAJORMINOR.la $SWFDEC_LIBS $GTK_LIBS"
AC_SUBST(SWFDEC_GTK_LIBS)
AC_SUBST(SWFDEC_GTK_CFLAGS)

GTK_DOC_CHECK([1.6])

if test "x${prefix}" = "xNONE"; then
  PACKAGE_PREFIX=${ac_default_prefix}
else
  PACKAGE_PREFIX=${prefix}
fi
AC_DEFINE_UNQUOTED(PACKAGE_PREFIX, "$PACKAGE_PREFIX", [Define the package prefix])
AC_SUBST(PACKAGE_PREFIX)

dnl #########################
dnl # Make the output files #
dnl #########################

dnl testsuite/autoplug/Makefile
dnl testsuite/Makefile
AC_CONFIG_FILES(
Makefile
doc/Makefile
libswfdec/Makefile
libswfdec/jpeg/Makefile
libswfdec/js/Makefile
libswfdec-gtk/Makefile
player/Makefile
test/Makefile
test/image/Makefile
test/sound/Makefile
test/trace/Makefile
test/various/Makefile
swfdec.pc
swfdec-gtk.pc
swfdec.spec
)

AC_OUTPUT

