AC_DEFUN(GST_DOC, [
AC_ARG_WITH(html-dir, AC_HELP_STRING([--with-html-dir=PATH], [path to installed docs]))

if test "x$with_html_dir" = "x" ; then
  HTML_DIR='${datadir}/gtk-doc/html'
else
  HTML_DIR=$with_html_dir
fi

AC_SUBST(HTML_DIR)

dnl check for gtk-doc
AC_CHECK_PROG(HAVE_GTK_DOC, gtkdoc-scangobj, true, false)
gtk_doc_min_version=0.7
if $HAVE_GTK_DOC ; then
    gtk_doc_version=`gtkdoc-mkdb --version`
    AC_MSG_CHECKING([gtk-doc version ($gtk_doc_version) >= $gtk_doc_min_version])
    if perl <<EOF ; then
      exit (("$gtk_doc_version" =~ /^[[0-9]]+\.[[0-9]]+$/) &&
            ("$gtk_doc_version" >= "$gtk_doc_min_version") ? 0 : 1);
EOF
      AC_MSG_RESULT(yes)
   else
      AC_MSG_RESULT(no)
      AC_MSG_ERROR(gtk-doc version is too low, need $gtk_doc_min_version, please disable doc building)
      HAVE_GTK_DOC=false
   fi
fi
# don't you love undocumented command line options?
GTK_DOC_SCANOBJ="gtkdoc-scangobj --nogtkinit"
AC_SUBST(HAVE_GTK_DOC)
AC_SUBST(GTK_DOC_SCANOBJ)

dnl check for docbook tools
AC_CHECK_PROG(HAVE_XSLTPROC, xsltproc, true, false)
AC_CHECK_PROG(HAVE_PDFTOPS, pdftops, true, false)
dnl this does not yet work properly, at least on debian -- wingo
HAVE_PDFXMLTEX=false

dnl check for image conversion tool
AC_CHECK_PROG(HAVE_FIG2DEV, fig2dev, true, false)

dnl The following is a hack: if fig2dev doesn't display an error message
dnl for the desired type, we assume it supports it.
HAVE_FIG2DEV_PNG=false
if test "x$HAVE_FIG2DEV" = "xtrue" ; then
  fig2dev_quiet=`fig2dev -L png </dev/null 2>&1 >/dev/null`
  if test "x$fig2dev_quiet" = "x" ; then
    HAVE_FIG2DEV_PNG=true
  fi
fi
HAVE_FIG2DEV_PDF=false
if test "x$HAVE_FIG2DEV" = "xtrue" ; then
  fig2dev_quiet=`fig2dev -L pdf </dev/null 2>&1 >/dev/null`
  if test "x$fig2dev_quiet" = "x" ; then
    HAVE_FIG2DEV_PDF=true
  fi
fi

AC_ARG_ENABLE(docs-build,
AC_HELP_STRING([--enable-docs-build][enable building of documentation]),
[case "${enableval}" in
  yes) if $HAVE_GTK_DOC; then BUILD_DOCS=yes; else AC_MSG_ERROR([you don't have gtk-doc, so don't use --docs-build]); fi; ;;
  no)  BUILD_DOCS=no ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-docs-build) ;;
esac],
[BUILD_DOCS=no]) dnl Default value

dnl AC_ARG_ENABLE(plugin-docs,
dnl [  --enable-plugin-docs         enable the building of plugin documentation
dnl                                (this is currently broken, so off by default)],
dnl [case "${enableval}" in
dnl   yes) BUILD_PLUGIN_DOCS=yes ;;
dnl   no)  BUILD_PLUGIN_DOCS=no ;;
dnl   *) AC_MSG_ERROR(bad value ${enableval} for --enable-plugin-docs) ;;
dnl esac], 
dnl [BUILD_PLUGIN_DOCS=no]) dnl Default value
BUILD_PLUGIN_DOCS=no

AM_CONDITIONAL(HAVE_GTK_DOC,        $HAVE_GTK_DOC)
AM_CONDITIONAL(BUILD_DOCS,          test "x$BUILD_DOCS" = "xyes")
AM_CONDITIONAL(BUILD_PLUGIN_DOCS,   test "x$BUILD_PLUGIN_DOCS" = "xyes")
AM_CONDITIONAL(HAVE_PDFXMLTEX,      $HAVE_PDFXMLTEX)
AM_CONDITIONAL(HAVE_PDFTOPS,        $HAVE_PDFTOPS)
AM_CONDITIONAL(HAVE_XSLTPROC,       $HAVE_XSLTPROC)
AM_CONDITIONAL(HAVE_FIG2DEV_PNG,    $HAVE_FIG2DEV_PNG)
AM_CONDITIONAL(HAVE_FIG2DEV_PDF,    $HAVE_FIG2DEV_PDF)

])

