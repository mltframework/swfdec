dnl autoconf configuration file for swfdec
AC_INIT
AC_CANONICAL_TARGET([])

dnl We disable static building for development, for time savings
dnl this goes before AS_LIBTOOL to appease autoconf
dnl *NOTE*: dnl this line before release, so release does static too
dnl AM_DISABLE_STATIC

dnl when going to/from release please set the nano (fourth number) right !
AS_VERSION(swfdec, SWFDEC, 0, 2, 2, 1)
SWF_DEBUG="-Wall -Werror"
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
AS_LIBTOOL(SWFDEC, 0, 1, 0, yes)

AM_CONFIG_HEADER(config.h)

dnl Add parameters for aclocal
dnl (This must come after AM_INIT_AUTOMAKE, since it modifies ACLOCAL)
ACLOCAL="$ACLOCAL -I m4"

AM_MAINTAINER_MODE

AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_AS
AS="${CC}"
AC_PROG_CXX
AC_PROG_CXXCPP
AC_ISC_POSIX

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

PKG_CHECK_MODULES(GLIB, glib-2.0, HAVE_GLIB=yes, HAVE_GLIB=no)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
if test "$HAVE_GLIB" = "no"; then
  AC_MSG_ERROR([cannot find GLIB-2.0, which is required for build])
fi

PKG_CHECK_MODULES(GTK, gtk+-2.0, HAVE_GTK=yes, HAVE_GTK=no)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
if test "$HAVE_GTK" = "no"; then
  AC_MSG_WARN([cannot find GTK+-2.0, player will be disabled])
fi
AM_CONDITIONAL(WITH_GTK,[test "$HAVE_GTK" != "no"])

AC_CHECK_PROG(HAVE_SDL, sdl-config, "yes", "no")
if test "$HAVE_SDL" = "yes"; then
  SDL_CFLAGS=`sdl-config --cflags`
  SDL_LIBS=`sdl-config --libs`
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
fi
AM_CONDITIONAL(WITH_SDL,[test "$HAVE_SDL" = "yes"])

PKG_CHECK_MODULES(LIBART, libart-2.0, HAVE_LIBART=yes, HAVE_LIBART=no)
AC_SUBST(LIBART_LIBS)
AC_SUBST(LIBART_CFLAGS)
if test "$HAVE_LIBART" = "no"; then
  AC_MSG_ERROR([cannot find libart-2.0, which is required for build])
fi

AC_CHECK_LIB(mad, mad_decoder_finish, HAVE_MAD="yes" MAD_LIBS="-lmad", HAVE_MAD="no")
AC_SUBST(MAD_LIBS)
if test "x$HAVE_MAD" = xyes; then
  AC_DEFINE(HAVE_MAD, 1, [Define if mad is enabled])
fi

MOZILLA_CFLAGS=
if test -z "$MOZILLA_CONFIG"; then
  AC_PATH_PROG(MOZILLA_CONFIG, mozilla-config, no)
fi
if test "x$MOZILLA_CONFIG" != "xno"; then
  _mozilla_include_dir=`mozilla-config --cflags|sed 's/-I\(.*\) .*/\1/'`
  MOZILLA_CFLAGS="-I$_mozilla_include_dir/plugin -I$_mozilla_include_dir/java -I$_mozilla_include_dir/nspr"
else
  AC_MSG_WARN([mozilla-config not found. Mozilla/Netscape plugin will not be built])
fi
AM_CONDITIONAL(WITH_MOZILLA,[test "$MOZILLA_CFLAGS" != ""])
AC_SUBST(MOZILLA_CFLAGS)

dnl GdkPixbuf loader

AC_PATH_PROG(QUERYLOADERS, gdk-pixbuf-query-loaders, true)

AC_ARG_ENABLE(pixbuf-loader,
    [  --enable-pixbuf-loader  Enable a SWF based GdkPixbuf loader [default=auto]],
    enable_pixbuf_loader="$enableval", enable_pixbuf_loader=auto)

if test x$enable_pixbuf_loader = xauto ; then
    if test x$QUERYLOADERS != x ; then 
        enable_pixbuf_loader=yes
    else
        enable_pixbuf_loader=no
    fi
fi

if test "x$enable_pixbuf_loader" = "xyes" ; then
  PKG_CHECK_MODULES(PIXBUF, gdk-pixbuf-2.0 >= 2.1.2)
  GTK_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
  AC_SUBST(PIXBUF_LIBS)
  AC_SUBST(PIXBUF_CFLAGS)
  AC_SUBST(GTK_VERSION)
else
  AC_MSG_WARN([GdkPixbuf >= 2.1.2 not found. Pixbuf loader will not be built])
fi

AM_CONDITIONAL(ENABLE_PIXBUF_LOADER, test x$enable_pixbuf_loader = xyes)

dnl a Gimp plugin

LIBGIMP_CFLAGS=""
LIBGIMP_LIBS=""
swf_gimp=no

PKG_CHECK_MODULES(LIBGIMP,[gimp-1.3 >= 1.3.10 gimpui-1.3 >= 1.3.10 gtk+-2.0 >= 2.0.0], test_gimp=true, test_gimp=false)
if test "x$test_gimp" = "xtrue"; then
      swf_gimp=yes
fi
AM_CONDITIONAL(ENABLE_GIMP_LOADER,[test $swf_gimp = yes])

AC_SUBST(LIBGIMP_CFLAGS)
AC_SUBST(LIBGIMP_LIBS)

dnl Check for X11 extensions
AC_PATH_XTRA
if test "-DX_DISPLAY_MISSING" = "$X_CFLAGS"; then
  AC_MSG_NOTICE([cannot find X11, the build system needs fixage])
fi
AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_LIBS)

AM_CONDITIONAL(HAVE_GNOME,          test "x$HAVE_GNOME" = "xyes")
AM_CONDITIONAL(HAVE_GTK,            test "x$HAVE_GTK" = "xyes")
AM_CONDITIONAL(HAVE_GTK_DOC,        $HAVE_GTK_DOC)

SWF_CFLAGS="$SWF_CFLAGS -I\$(top_srcdir)/libswfdec $SWF_DEBUG $GLIB_CFLAGS"
SWF_LIBS="$SWF_LIBS \$(top_builddir)/libswfdec/libswfdec.la $MAD_LIBS $GLIB_LIBS -lz"
AC_SUBST(SWF_LIBS)
AC_SUBST(SWF_CFLAGS)

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
libswfdec/Makefile
libswfdec/mpglib/Makefile
libswfdec/jpeg/Makefile
test/Makefile
plugin/Makefile
player/Makefile
pixbuf-loader/Makefile
gimp-loader/Makefile
swfdec.pc
swfdec.spec
)

AC_OUTPUT

