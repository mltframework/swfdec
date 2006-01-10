AC_PREREQ([2.58])
AC_INIT(swfdec,0.3.4.1)

[nano=$(echo $PACKAGE_VERSION | sed 's/[0-9]\.[0-9]\.[0-9]\.*//')]
if test x"$nano" = x1 ; then
  SWFDEC_CVS="yes"
else
  SWFDEC_CVS="no"
fi

AM_INIT_AUTOMAKE(1.6)
dnl AC_CANONICAL_TARGET([])
AM_MAINTAINER_MODE

SWFDEC_MAJORMINOR=0.3
AC_SUBST(SWFDEC_MAJORMINOR)

AM_CONFIG_HEADER(config.h)

GLOBAL_CFLAGS="-Wall"
if test x$SWFDEC_CVS = xyes ; then
  GLOBAL_CFLAGS="$GLOBAL_CFLAGS -Werror"
fi

SWFDEC_LIBVERSION="0:0:0"
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

WITH_GLIB_COMPAT=no
AC_ARG_ENABLE(glib-compat,
  AC_HELP_STRING([--enable-glib-compat],
    [uses internal glib compatibility code]),
    [WITH_GLIB_COMPAT=${enableval}])
AM_CONDITIONAL(WITH_GLIB_COMPAT, [test x"$WITH_GLIB_COMPAT" = xyes ])

if test "x$WITH_GLIB_COMPAT" != "xyes" ; then
  GLIB_REQUIRES=glib-2.0
  PKG_CHECK_MODULES(GLIB, glib-2.0 gobject-2.0, HAVE_GLIB=yes, HAVE_GLIB=no)
  if test "$HAVE_GLIB" = "no"; then
    AC_MSG_ERROR([cannot find GLIB-2.0, which is required for build])
  fi
else
  GLIB_REQUIRES=
  GLIB_LIBS="\$(top_builddir)/glib-compat/glib/libglib.la"
  GLIB_CFLAGS="-I\$(top_srcdir)/glib-compat"
fi
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_REQUIRES)

PKG_CHECK_MODULES(GTK, gtk+-2.0, HAVE_GTK=yes, HAVE_GTK=no)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
if test "$HAVE_GTK" = "no"; then
  AC_MSG_WARN([cannot find GTK+-2.0, player will be disabled])
fi
AM_CONDITIONAL(WITH_GTK,[test "$HAVE_GTK" != "no"])

PKG_CHECK_MODULES(LIBOIL, liboil-0.3 >= 0.3.1.1, HAVE_LIBOIL=yes, HAVE_LIBOIL=no)
AC_SUBST(LIBOIL_LIBS)
AC_SUBST(LIBOIL_CFLAGS)
if test "$HAVE_LIBOIL" = "no"; then
  AC_MSG_ERROR([cannot find liboil-0.3, which is required for build])
fi

AC_ARG_WITH(backend,
    [  --with-backend=BACKEND  Use a particular rendering backend [default=cairo]],
    BACKEND="$withval", BACKEND=cairo)

if test "x$BACKEND" = "xlibart" ; then
  PKG_CHECK_MODULES(LIBART, libart-2.0, HAVE_LIBART=yes, HAVE_LIBART=no)
  AC_SUBST(LIBART_LIBS)
  AC_SUBST(LIBART_CFLAGS)
  if test "$HAVE_LIBART" = "no"; then
    AC_MSG_ERROR([cannot find libart-2.0, which is required for build])
  else
    AC_DEFINE(HAVE_LIBART, 1, [Define if swfdec uses libart for rendering])
  fi
fi
AM_CONDITIONAL(HAVE_LIBART, test x$HAVE_LIBART = xyes)

if test "x$BACKEND" = "xcairo" ; then
  PKG_CHECK_MODULES(CAIRO, cairo >= 0.4.0, HAVE_CAIRO=yes, HAVE_CAIRO=no)
  AC_SUBST(CAIRO_LIBS)
  AC_SUBST(CAIRO_CFLAGS)
  if test "$HAVE_CAIRO" = "no"; then
    AC_MSG_ERROR([cannot find cairo, which is required for build])
  else
    AC_DEFINE(HAVE_CAIRO, 1, [Define if swfdec uses cairo for rendering])
  fi
fi
AM_CONDITIONAL(HAVE_CAIRO, test x$HAVE_CAIRO = xyes)

AC_CHECK_LIB(mad, mad_decoder_finish, HAVE_MAD="yes" MAD_LIBS="-lmad", HAVE_MAD="no")
AC_SUBST(MAD_LIBS)
if test "x$HAVE_MAD" = xyes; then
  AC_DEFINE(HAVE_MAD, 1, [Define if mad is enabled])
fi

PKG_CHECK_MODULES(MOZILLA, mozilla-plugin, HAVE_MOZILLA=yes, HAVE_MOZILLA=no)
if test "$HAVE_MOZILLA" = no ; then
  PKG_CHECK_MODULES(MOZILLA, firefox-plugin, HAVE_MOZILLA=yes, HAVE_MOZILLA=no)
fi
AC_SUBST(MOZILLA_CFLAGS)
AC_SUBST(MOZILLA_LIBS)
AM_CONDITIONAL(HAVE_MOZILLA, [test $HAVE_MOZILLA = yes])
if test "x$HAVE_MOZILLA" = "xno"; then
  AC_MSG_WARN([mozilla-config not found. Mozilla/Netscape plugin will not be built])
fi

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

PKG_CHECK_MODULES(LIBGIMP,[gimp-2.0 gimpui-2.0 gtk+-2.0 >= 2.0.0],
  HAVE_GIMP=yes, HAVE_GIMP=no)
if test "x$HAVE_GIMP" = xyes ; then
  gimp_prefix=$(pkg-config --variable=prefix gimp-2.0)
  if test "x$gimp_prefix" = "x$prefix" ; then
    gimplibdir=`pkg-config --define-variable=prefix='${prefix}' --variable=gimplibdir gimp-2.0`
  else
    HAVE_GIMP=no
    AC_MSG_WARN([gimp plugin disabled because gimp prefix is not the same as installation prefix])
  fi
fi
AM_CONDITIONAL(HAVE_GIMP,[test $HAVE_GIMP = yes])
AC_SUBST(gimplibdir)

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

PKG_CHECK_MODULES(GST, gstreamer-0.10 > 0.10.0 gstreamer-plugins-base-0.10,
  HAVE_GST=yes, HAVE_GST=no)
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)
AM_CONDITIONAL(HAVE_GST, [test $HAVE_GST = yes])

AC_SUBST(GLOBAL_CFLAGS)

SWF_CFLAGS="$SWF_CFLAGS -I\$(top_srcdir)/libswfdec $GLIB_CFLAGS"
SWF_LIBS="$SWF_LIBS \$(top_builddir)/libswfdec/libswfdec-$SWFDEC_MAJORMINOR.la $MAD_LIBS $GLIB_LIBS -lz"
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
glib-compat/Makefile
glib-compat/glib/Makefile
libswfdec/Makefile
libswfdec/jpeg/Makefile
libswfdec/js/Makefile
libswfdec/mpeg/Makefile
test/Makefile
plugin/Makefile
pixbuf-loader/Makefile
gimp-loader/Makefile
swfdec.pc
swfdec.spec
)

AC_OUTPUT

