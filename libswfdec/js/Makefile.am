noinst_LTLIBRARIES = libjs.la

libjs_la_SOURCES = \
	js.msg \
	jsapi.c \
	jsapi.h \
	jsarena.c \
	jsarena.h \
	jsarray.c \
	jsarray.h \
	jsatom.c \
	jsatom.h \
	jsbit.h \
	jsbool.c \
	jsbool.h \
	jsclist.h \
	jscntxt.c \
	jscntxt.h \
	jscompat.h \
	jsconfig.h \
	jsdate.c \
	jsdate.h \
	jsdbgapi.c \
	jsdbgapi.h \
	jsdhash.c \
	jsdhash.h \
	jsdtoa.c \
	jsdtoa.h \
	jsemit.c \
	jsemit.h \
	jsexn.c \
	jsexn.h \
	jsfile.c \
	jsfile.h \
	jsfun.c \
	jsfun.h \
	jsgc.c \
	jsgc.h \
	jshash.c \
	jshash.h \
	jsinterp.c \
	jsinterp.h \
	jslibmath.h \
	jslock.c \
	jslock.h \
	jslog2.c \
	jslong.c \
	jslong.h \
	jsmath.c \
	jsmath.h \
	jsnum.c \
	jsnum.h \
	jsobj.c \
	jsobj.h \
	jsopcode.c \
	jsopcode.h \
	jsopcode.tbl \
	jsosdep.h \
	jsotypes.h \
	jsparse.c \
	jsparse.h \
	jsprf.c \
	jsprf.h \
	jsprvtd.h \
	jspubtd.h \
	jsregexp.c \
	jsregexp.h \
	jsscan.c \
	jsscan.h \
	jsscope.c \
	jsscope.h \
	jsscript.c \
	jsscript.h \
	jsstddef.h \
	jsstr.c \
	jsstr.h \
	jstypes.h \
	jsutil.c \
	jsutil.h \
	jsxdrapi.c \
	jsxdrapi.h \
	prmjtime.c \
	prmjtime.h

noinst_PROGRAMS = jscpucfg
jscpucfg_SOURCES = \
	jscpucfg.c  \
	jscpucfg.h

libjs_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(GLIB_CFLAGS) -I. \
	-DXP_UNIX
libjs_la_LDFLAGS = -lm

CLEANFILES = jsautocfg.h

jsapi.c: jsautocfg.h

jsautocfg.h: jscpucfg
	./jscpucfg >jsautocfg.h


