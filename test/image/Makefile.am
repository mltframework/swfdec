check_PROGRAMS = image
TESTS = $(check_PROGRAMS)

image_SOURCES = image.c
image_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) $(CAIRO_CFLAGS)
image_LDFLAGS = $(SWFDEC_LIBS) $(CAIRO_LIBS)

EXTRA_DIST = \
	README \
	bw.jpg \
	color-transform-add80.swf \
	color-transform-add80.swf.png \
	color-transform-add80-alpha.swf \
	color-transform-add80-alpha.swf.png \
	duplicate-depth.c \
	duplicate-depth-5.swf \
	duplicate-depth-5.swf.png \
	duplicate-depth-6.swf \
	duplicate-depth-6.swf.png \
	duplicate-depth-7.swf \
	duplicate-depth-7.swf.png \
	image-jpeg-alpha.swf \
	image-jpeg-alpha.swf.png \
	image-lossless-alpha.swf \
	image-lossless-alpha.swf.png \
	negative-color-transform.c \
	negative-color-transform.swf \
	negative-color-transform.swf.png
