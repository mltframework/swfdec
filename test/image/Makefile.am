check_PROGRAMS = image
TESTS = $(check_PROGRAMS)

image_SOURCES = image.c
image_CFLAGS = $(GLOBAL_CFLAGS) $(SWF_CFLAGS) $(CAIRO_CFLAGS)
image_LDFLAGS = $(SWF_LIBS) $(CAIRO_LIBS)

EXTRA_DIST = \
	README \
	color-transform-add80.swf \
	color-transform-add80.swf.png \
	color-transform-add80-alpha.swf \
	color-transform-add80-alpha.swf.png \
	image-jpeg-alpha.swf \
	image-jpeg-alpha.swf.png \
	image-lossless-alpha.swf \
	image-lossless-alpha.swf.png
