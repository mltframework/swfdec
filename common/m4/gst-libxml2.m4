AC_DEFUN(GST_LIBXML2_CHECK, [
dnl Minimum required version of libxml2
LIBXML2_REQ="2.4.0"
AC_SUBST(LIBXML2_REQ)

dnl check for libxml2
LIBXML_PKG=', libxml-2.0'
PKG_CHECK_MODULES(XML, libxml-2.0 >= $LIBXML2_REQ, HAVE_LIBXML2=yes, HAVE_LIBXML2=no)
if test "x$HAVE_LIBXML2" = "xyes"; then
  AC_DEFINE(HAVE_LIBXML2, 1, [Define if libxml2 is available])
else
  AC_MSG_ERROR([Need libxml2 for glib2 builds -- you should be able to do without it -- this needs fixing])
fi
AC_SUBST(LIBXML_PKG)
AC_SUBST(XML_LIBS)
AC_SUBST(XML_CFLAGS)
])
