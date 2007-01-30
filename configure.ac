AC_PREREQ([2.58])
AC_INIT(swfdec,0.4.1.1)

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
fi

SWFDEC_LIBVERSION="1:0:0"
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

GLIB_REQUIRES=2.0
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRES gobject-2.0 >= $GLIB_REQUIRES, HAVE_GLIB=yes, HAVE_GLIB=no)
if test "$HAVE_GLIB" = "no"; then
  AC_MSG_ERROR([cannot find GLIB-2.0, which is required for build])
fi
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_REQUIRES)
dnl FIXME: detect these executables correctly
GLIB_GENMARSHAL=glib-genmarshal
AC_SUBST(GLIB_GENMARSHAL)
GLIB_MKENUMS=glib-mkenums
AC_SUBST(GLIB_MKENUMS)

PKG_CHECK_MODULES(PANGO, pangocairo, HAVE_PANGO=yes, HAVE_PANGO=no)
if test "$HAVE_PANGO" = "no"; then
  AC_MSG_ERROR([cannot find pangocairo, which is required for build])
fi
AC_SUBST(PANGO_LIBS)
AC_SUBST(PANGO_CFLAGS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.8.0, HAVE_GTK=yes, HAVE_GTK=no)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
if test "$HAVE_GTK" = "no"; then
  AC_MSG_WARN([cannot find GTK+-2.0, player will be disabled])
fi
AM_CONDITIONAL(WITH_GTK,[test "$HAVE_GTK" != "no"])

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
    AC_MSG_WARN([no alsa audio support])
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

PKG_CHECK_MODULES(LIBOIL, liboil-0.3 >= 0.3.1.1, HAVE_LIBOIL=yes, HAVE_LIBOIL=no)
AC_SUBST(LIBOIL_LIBS)
AC_SUBST(LIBOIL_CFLAGS)
if test "$HAVE_LIBOIL" = "no"; then
  AC_MSG_ERROR([cannot find liboil-0.3, which is required for build])
fi

PKG_CHECK_MODULES(CAIRO, cairo >= 0.4.0, HAVE_CAIRO=yes, HAVE_CAIRO=no)
AC_SUBST(CAIRO_LIBS)
AC_SUBST(CAIRO_CFLAGS)
if test "$HAVE_CAIRO" = "no"; then
  AC_MSG_ERROR([cannot find cairo, which is required for build])
fi

AC_CHECK_LIB(mad, mad_decoder_finish, HAVE_MAD="yes" MAD_LIBS="-lmad", HAVE_MAD="no")
AC_SUBST(MAD_LIBS)
if test "x$HAVE_MAD" = xyes; then
  AC_DEFINE(HAVE_MAD, 1, [Define if mad is enabled])
fi
AM_CONDITIONAL(HAVE_MAD, [test "x$HAVE_MAD" = xyes])

dnl  I'd put a required version in here if distros can agree on a way
dnl  to detect ffmpeg. But as it stands even pkg-config versions are weird.
dnl  So you'll have to update your ffmpeg checkout if compilation fails.
dnl  Or you submit a patch that detects ffmpeg reliably on the distros.
PKG_CHECK_MODULES(FFMPEG, libavcodec, HAVE_FFMPEG=yes, HAVE_FFMPEG=no)
AC_SUBST(FFMPEG_CFLAGS)
AC_SUBST(FFMPEG_LIBS)
if test "x$HAVE_FFMPEG" = xyes; then
  AC_DEFINE(HAVE_FFMPEG, 1, [Define if ffmpeg is enabled])
fi
AM_CONDITIONAL(HAVE_FFMPEG, [test "x$HAVE_FFMPEG" = xyes])

AC_SUBST(GLOBAL_CFLAGS)

SWF_CFLAGS="$SWF_CFLAGS -I\$(top_srcdir) $GLIB_CFLAGS"
SWF_LIBS="$SWF_LIBS \$(top_builddir)/libswfdec/libswfdec-$SWFDEC_MAJORMINOR.la $MAD_LIBS $GLIB_LIBS -lz"
AC_SUBST(SWF_LIBS)
AC_SUBST(SWF_CFLAGS)

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
player/Makefile
test/Makefile
test/sound/Makefile
test/trace/Makefile
test/various/Makefile
swfdec.pc
swfdec.spec
)

AC_OUTPUT

