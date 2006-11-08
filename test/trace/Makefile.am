check_PROGRAMS = trace
TESTS = $(check_PROGRAMS)

trace_SOURCES = trace.c

trace_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS)
trace_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

EXTRA_DIST = \
	case1-6.swf \
	case1-6.swf.trace \
	case1-7.swf \
	case1-7.swf.trace \
	children.swf \
	children.swf.trace \
	countdown.swf \
	countdown.swf.trace \
	currentframe.swf \
	currentframe.swf.trace \
	goto1.swf \
	goto1.swf.trace \
	goto2.swf \
	goto2.swf.trace \
	goto3.swf \
	goto3.swf.trace \
	goto4.swf \
	goto4.swf.trace \
	load-4.swf \
	load-4.swf.trace \
	load-5.swf \
	load-5.swf.trace \
	order.swf \
	order.swf.trace \
	setvariable.swf \
	setvariable.swf.trace

CLEANFILES = tmp
