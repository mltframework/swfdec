AC_PREREQ([2.58])

m4_define(swfdec_major, 0)
m4_define(swfdec_minor, 9)
m4_define(swfdec_micro, 3)
AC_INIT(swfdec,[swfdec_major.swfdec_minor.swfdec_micro])

SWFDEC_VERSION_MAJOR=swfdec_major
SWFDEC_VERSION_MINOR=swfdec_minor
SWFDEC_VERSION_MICRO=swfdec_micro
SWFDEC_VERSION=$SWFDEC_VERSION_MAJOR.$SWFDEC_VERSION_MINOR.$SWFDEC_VERSION_MICRO
AC_SUBST(SWFDEC_VERSION_MAJOR)
AC_SUBST(SWFDEC_VERSION_MINOR)
AC_SUBST(SWFDEC_VERSION_MICRO)
AC_SUBST(SWFDEC_VERSION)


[SWFDEC_DEVEL=$(echo $SWFDEC_VERSION_MICRO | sed 's/[0-9]*[13579]/yes/')]
[SWFDEC_SNAPSHOT=$(echo $SWFDEC_VERSION_MINOR | sed 's/[0-9]*[13579]/yes/')]

AM_INIT_AUTOMAKE(1.6)
dnl AC_CANONICAL_TARGET([])

SWFDEC_MAJORMINOR=$SWFDEC_VERSION_MAJOR.$SWFDEC_VERSION_MINOR
AC_SUBST(SWFDEC_MAJORMINOR)

AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl decide on error flags
dnl if we support them, we set them unconditionally
AS_COMPILER_FLAGS(GLOBAL_CFLAGS, "-Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wold-style-definition -Wdeclaration-after-statement -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wmissing-noreturn -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wformat-nonliteral -Wformat-security -Wswitch-enum -Wswitch-default -Winit-self -Wmissing-include-dirs -Wundef -Waggregate-return -Wmissing-format-attribute -Wnested-externs -Wunsafe-loop-optimizations -Wpacked -Winvalid-pch -Wsync-nand -Wlogical-op")
dnl if we're in nano >= 1, add -Werror if supported
if test x$SWFDEC_DEVEL = xyes ; then
  AS_COMPILER_FLAGS(GLOBAL_CFLAGS, "-Werror")
  DEFAULT_DEBUG_LEVEL="SWFDEC_LEVEL_WARNING"
else
  DEFAULT_DEBUG_LEVEL="SWFDEC_LEVEL_ERROR"
fi
AC_DEFINE_UNQUOTED(SWFDEC_LEVEL_DEFAULT, $DEFAULT_DEBUG_LEVEL, [Default debug level used])

dnl Compute the libversion automatically, so there's no need to think about it
dnl when bumping versions.
dnl Note that the libversions are based on Swfdec's current release mechanism 
dnl detailed at 
dnl http://lists.freedesktop.org/archives/swfdec/2008-January/001131.html
dnl If this ever changes (like when Swfdec gets a stable API), don't forget to
dnl change this
if test x"$SWFDEC_SNAPSHOT" = xyes ; then
  SWFDEC_LIBVERSION="$SWFDEC_VERSION_MICRO:0:0"
else
  SWFDEC_LIBVERSION="0:0:0"
fi
AC_SUBST(SWFDEC_LIBVERSION)
AM_PROG_LIBTOOL

dnl C99 is only required to get definitions for NAN and INFINITY.
AS_COMPILER_FLAGS(GLOBAL_CFLAGS, "-std=gnu99")

dnl ensures the library is linked against the internal Mozilla
dnl if this doesn't work on your platform, I'll take patches :)
SYMBOLIC_LDFLAGS="-Wl,-Bsymbolic"
AC_SUBST(SYMBOLIC_LDFLAGS)

dnl Add parameters for aclocal
dnl (This must come after AM_INIT_AUTOMAKE, since it modifies ACLOCAL)
#ACLOCAL_AMFLAGS="-I m4 $ACLOCAL_AMFLAGS"

AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AM_PROG_AS
AC_PROG_AWK

AC_HEADER_STDC

AC_MSG_CHECKING([for timezone support])
AC_COMPILE_IFELSE([
  AC_LANG_PROGRAM([[
    @%:@include <time.h>
  ]], [[
    time_t t;
    localtime (&t)->tm_gmtoff;
  ]])],
  HAVE_TIMEZONE="yes",
  HAVE_TIMEZONE="no")
if test "x$HAVE_TIMEZONE" = "xyes"; then
  AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_TIMEZONE, 1, [Define if we can query the timezone])
else
  AC_MSG_RESULT([no])
fi


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

GLIB_VER=2.16
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_VER gobject-2.0 >= $GLIB_VER gthread-2.0 >= $GLIB_VER,
		HAVE_GLIB=yes, HAVE_GLIB=no)
if test "$HAVE_GLIB" = "no"; then
  AC_MSG_ERROR([glib-2.0, gobject-2.0 and gthread-2.0 >= $GLIB_VER are required to build swfdec])
fi
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_VER)
dnl FIXME: detect these executables correctly
GLIB_GENMARSHAL=glib-genmarshal
AC_SUBST(GLIB_GENMARSHAL)
GLIB_MKENUMS=glib-mkenums
AC_SUBST(GLIB_MKENUMS)

PANGO_VER=1.16
PKG_CHECK_MODULES(PANGO, pangocairo >= $PANGO_VER, HAVE_PANGO=yes, HAVE_PANGO=no)
if test "$HAVE_PANGO" = "no"; then
  AC_MSG_ERROR([pangocairo >= $PANGO_VER is required to build swfdec])
fi
AC_SUBST(PANGO_LIBS)
AC_SUBST(PANGO_CFLAGS)

AC_ARG_ENABLE(gtk,
	AS_HELP_STRING([--enable-gtk],
			[enable build of swfdec-gtk (default=yes)])],
	enable_gtk=$enableval,
	enable_gtk="yes")

dnl 
dnl GTK: We want this for swfdec-gtk
dnl
GTK_VER=2.8.0
AC_SUBST(GTK_VER)
if test "$enable_gtk" = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_VER libsoup-2.4, HAVE_GTK=yes)
	AC_DEFINE(HAVE_GTK, 1, [Define if Gtk is enabled])
else
	AC_MSG_NOTICE([Gtk support was not enabled.])
fi
AM_CONDITIONAL(WITH_GTK, [test "x$HAVE_GTK" = xyes])

dnl
dnl audio backend
dnl
AC_ARG_WITH(audio,
            [AC_HELP_STRING([--with-audio=@<:@auto/pulse/none@:>@],
                            [audio backend to use])],,
	    [with_audio=alsa])

AUDIO_TYPE=
if test "$with_audio" = "alsa"; then
  PKG_CHECK_MODULES(ALSA, alsa >= 1.0, AUDIO_TYPE=alsa)
  if test "$AUDIO_TYPE" = "alsa"; then
    AUDIO_CFLAGS=$ALSA_CFLAGS
    AUDIO_LIBS=$ALSA_LIBS
  else
    if test "$with_audio" = "alsa"; then
      AC_MSG_ERROR([no alsa audio support])
    else
      AC_MSG_WARN([no alsa audio support])
    fi
  fi
fi

dnl Use PA if ALSA wasn't found and we're auto and it's available.
dnl Disabled for now until the PA backend works.
if test "$with_audio" = "pulse"; then
  PKG_CHECK_MODULES(PA, libpulse-mainloop-glib, AUDIO_TYPE="pulse")
  if test "$AUDIO_TYPE" = "pulse"; then
    AUDIO_CFLAGS=$PA_CFLAGS
    AUDIO_LIBS=$PA_LIBS
  else
    if test "$with_audio" = "pulse"; then
      AC_MSG_ERROR([no pulseaudio support])
    else
      AC_MSG_WARN([no pulseaudio support])
    fi
  fi
fi

dnl If all else fails, fall back to none.
if test "$with_audio" = "none"; then
  AUDIO_CFLAGS=
  AUDIO_LIBS=
  AUDIO_TYPE=none
fi

if test "x$AUDIO_TYPE" = "x"; then
  AC_MSG_ERROR([audio backend "$with_audio" could not be used])
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

CAIRO_VER=1.6.0
PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_VER cairo-png >= $CAIRO_VER, HAVE_CAIRO=yes, HAVE_CAIRO=no)
AC_SUBST(CAIRO_LIBS)
AC_SUBST(CAIRO_CFLAGS)
if test "$HAVE_CAIRO" = "no"; then
  AC_MSG_ERROR([cairo and cairo-png = $CAIRO_VER is required to build swfdec])
fi

AC_ARG_ENABLE(gstreamer,
	AS_HELP_STRING([--enable-gstreamer],
			[enable GStreamer support (default=yes)])],
	enable_gstreamer=$enableval,
	enable_gstreamer="yes")

if test "$enable_gstreamer" = "yes"; then
	GST_REQUIRED=0.10.11
	GST_PB_REQUIRED=0.10.15
	PKG_CHECK_MODULES(GST, gstreamer-0.10 >= $GST_REQUIRED gstreamer-pbutils-0.10 >= $GST_PB_REQUIRED, HAVE_GST=yes, HAVE_GST=no)
	if test "x$HAVE_GST" = xyes; then
	  AC_DEFINE(HAVE_GST, 1, [Define if GStreamer is enabled])
	else
	  AC_MSG_ERROR([Couldn't find GStreamer $GST_REQUIRED and gstreamer-pbutils $GST_PB_REQUIRED.])
	fi
else
	AC_MSG_NOTICE([GStreamer support was not enabled.])
fi
AM_CONDITIONAL(HAVE_GST, [test "x$HAVE_GST" = xyes])


AC_ARG_ENABLE(vivified,
	AS_HELP_STRING([--enable-vivified],
			[enable Vivified Flash debugger (default=no)])],
	enable_vivi=$enableval,
	enable_vivi="no")
if test "$enable_vivi" = "yes"; then
	AM_PROG_LEX
	MING_REQUIRED=0.4.0.beta5
	VIVI_GTK_REQUIRED=2.11.6
	VIVI_GLIB_REQUIRED=2.14
	PKG_CHECK_MODULES(VIVI, libming >= $MING_REQUIRED gmodule-export-2.0 gtk+-2.0 >= $VIVI_GTK_REQUIRED glib-2.0 >= $VIVI_GLIB_REQUIRED, HAVE_VIVI=yes, HAVE_VIVI=no)
	if test "x$HAVE_VIVI" = xyes; then
	  AC_DEFINE(HAVE_VIVI, 1, [Define if Vivified is enabled])
	else
	  AC_MSG_ERROR([Vivified requirements not met. You need libming >= $MING_REQUIRED, glib >= $VIVI_GLIB_REQUIRED and Gtk+ >= $VIVI_GTK_REQUIRED.])
	fi
else
	AC_MSG_NOTICE([Vivified was not enabled.])
fi
AM_CONDITIONAL(HAVE_VIVI, [test "x$HAVE_VIVI" = xyes])


AC_SUBST(GLOBAL_CFLAGS)
AC_SUBST(GLOBAL_CFLAGS)

SWFDEC_CFLAGS="-I\$(top_srcdir) $GLIB_CFLAGS $CAIRO_CFLAGS"
SWFDEC_LIBS="\$(top_builddir)/swfdec/libswfdec-$SWFDEC_MAJORMINOR.la $GLIB_LIBS $CAIRO_LIBS -lz -lm"
AC_SUBST(SWFDEC_LIBS)
AC_SUBST(SWFDEC_CFLAGS)

SWFDEC_GTK_CFLAGS="$SWFDEC_CFLAGS $GTK_CFLAGS"
SWFDEC_GTK_LIBS="\$(top_builddir)/swfdec-gtk/libswfdec-gtk-$SWFDEC_MAJORMINOR.la $SWFDEC_LIBS $GTK_LIBS"
AC_SUBST(SWFDEC_GTK_LIBS)
AC_SUBST(SWFDEC_GTK_CFLAGS)

GTK_DOC_CHECK([1.10])

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
data/Makefile
data/swfdec.pc
data/swfdec-gtk.pc
doc/Makefile
swfdec/Makefile
swfdec/swfdec_version.h
swfdec/jpeg/Makefile
swfdec-gtk/Makefile
player/Makefile
test/Makefile
test/custom/Makefile
test/gnash/Makefile
test/gnash/media/Makefile
test/gnash/actionscript.all/Makefile
test/gnash/misc-mtasc.all/Makefile
test/gnash/misc-swfc.all/Makefile
test/gnash/misc-swfmill.all/Makefile
test/image/Makefile
test/sound/Makefile
test/trace/Makefile
test/various/Makefile
tools/Makefile
vivified/Makefile
vivified/code/Makefile
vivified/code/test/Makefile
vivified/code/test/compiler/Makefile
vivified/code/test/decompiler/Makefile
)

AC_OUTPUT

