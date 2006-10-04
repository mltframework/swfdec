check_PROGRAMS = trace
TESTS = $(check_PROGRAMS)

trace_SOURCES = trace.c

trace_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS)
trace_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

EXTRA_DIST = \
	children.swf \
	children.swf.trace \
	countdown.swf \
	countdown.swf.trace \
	load-4.swf \
	load-4.swf.trace \
	load-5.swf \
	load-5.swf.trace \
	order.swf \
	order.swf.trace

