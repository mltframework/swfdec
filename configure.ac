dnl autoconf configuration file for swfdec
AC_INIT
AC_CANONICAL_TARGET([])

dnl We disable static building for development, for time savings
dnl this goes before AS_LIBTOOL to appease autoconf
dnl *NOTE*: dnl this line before release, so release does static too
dnl AM_DISABLE_STATIC

dnl when going to/from release please set the nano (fourth number) right !
AS_VERSION(swfdec, SWFDEC, 0, 1, 3, 1)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
AS_LIBTOOL(SWFDEC, 0, 0, 0, yes)

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

dnl Set up conditionals for (target) architecture:
dnl ==============================================

dnl Determine CPU
case "x${target_cpu}" in
  xi?86 | k?) HAVE_CPU_I386=yes
              AC_DEFINE(HAVE_CPU_I386, 1, [Define if the target CPU is an x86])
              dnl FIXME could use some better detection
              dnl       (ie CPUID)
              case "x${target_cpu}" in
                xi386 | xi486) ;;
                *)             AC_DEFINE(HAVE_RDTSC, 1, [Define if RDTSC is available]) ;;
              esac ;;
  xpowerpc)   HAVE_CPU_PPC=yes
              AC_DEFINE(HAVE_CPU_PPC, 1, [Define if the target CPU is a PPC]) ;;
  xalpha)     HAVE_CPU_ALPHA=yes
              AC_DEFINE(HAVE_CPU_ALPHA, 1, [Define if the target CPU is an Alpha]) ;;
  xarm*)      HAVE_CPU_ARM=yes
              AC_DEFINE(HAVE_CPU_ARM, 1, [Define if the target CPU is an ARM]) ;;
  xsparc*)    HAVE_CPU_SPARC=yes
              AC_DEFINE(HAVE_CPU_SPARC, 1, [Define if the target CPU is a PPC]) ;;
  xmips*)     HAVE_CPU_MIPS=yes
              AC_DEFINE(HAVE_CPU_MIPS, 1, [Define if the target CPU is a MIPS]) ;;
  xhppa*)     HAVE_CPU_HPPA=yes
              AC_DEFINE(HAVE_CPU_HPPA, 1, [Define if the target CPU is a HPPA]) ;;
esac

dnl Determine endianness
AC_C_BIGENDIAN

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
  AC_MSG_ERROR([cannot find GTK+-2.0, which is required for build])
fi

PKG_CHECK_MODULES(LIBART, libart-2.0, HAVE_LIBART=yes, HAVE_LIBART=no)
AC_SUBST(LIBART_LIBS)
AC_SUBST(LIBART_CFLAGS)
if test "$HAVE_LIBART" = "no"; then
  AC_MSG_ERROR([cannot find libart-2.0, which is required for build])
fi

if test -z "$MOZILLA_CONFIG"; then
  AC_PATH_PROG(MOZILLA_CONFIG, mozilla-config, no)
fi
if test "$MOZILLA_CONFIG" = "no"; then
  AC_MSG_ERROR([cannot find mozilla development headers, which is required for build])
fi
_mozilla_include_dir=`mozilla-config --cflags|sed 's/-I\(.*\) .*/\1/'`
MOZILLA_CFLAGS="-I$_mozilla_include_dir/plugin -I$_mozilla_include_dir/java -I$_mozilla_include_dir/nspr"
AC_SUBST(MOZILLA_CFLAGS)

dnl Check for X11 extensions
AC_PATH_XTRA
if test "-DX_DISPLAY_MISSING" = "$X_CFLAGS"; then
  AC_MSG_NOTICE([cannot find X11, the build system needs fixage])
fi
AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_LIBS)

dnl ######################################################################
dnl # Check command line parameters, and set shell variables accordingly #
dnl ######################################################################

dnl #############################
dnl # Set automake conditionals #
dnl #############################

dnl These should be "USE_*" instead of "HAVE_*", but some packages expect
dnl HAVE_ and it is likely to be easier to stick with the old name
AM_CONDITIONAL(HAVE_CPU_I386,       test "x$HAVE_CPU_I386" = "xyes")
AM_CONDITIONAL(HAVE_CPU_PPC,        test "x$HAVE_CPU_PPC" = "xyes")
AM_CONDITIONAL(HAVE_CPU_ALPHA,      test "x$HAVE_CPU_ALPHA" = "xyes")
AM_CONDITIONAL(HAVE_CPU_ARM,        test "x$HAVE_CPU_ARM" = "xyes")
AM_CONDITIONAL(HAVE_CPU_SPARC,      test "x$HAVE_CPU_SPARC" = "xyes")
AM_CONDITIONAL(HAVE_LIBMMX,         test "x$USE_LIBMMX" = "xyes")

AM_CONDITIONAL(EXPERIMENTAL,        test "$EXPERIMENTAL" = "$xyes")
AM_CONDITIONAL(BROKEN,              test "$BROKEN" = "$xyes")

AM_CONDITIONAL(HAVE_GNOME,          test "x$HAVE_GNOME" = "xyes")
AM_CONDITIONAL(HAVE_GTK,            test "x$HAVE_GTK" = "xyes")
AM_CONDITIONAL(HAVE_GTK_DOC,        $HAVE_GTK_DOC)

SWF_CFLAGS="$SWF_CFLAGS -I\$(top_srcdir)/libswfdec -Wall $GLIB_CFLAGS"
SWF_LIBS="$SWF_LIBS \$(top_builddir)/libswfdec/libswfdec.la $GLIB_LIBS -lz"
AC_SUBST(SWF_LIBS)
AC_SUBST(SWF_CFLAGS)

CFLAGS="$CFLAGS -Wall"
AC_SUBST(CFLAGS)

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
swfdec.pc
swfdec.spec
)

AC_OUTPUT

