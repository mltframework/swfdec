
#include <glib.h>

#include "jpeg_rgb_internal.h"
#include "jpeg.h"


static void convert(JpegRGBDecoder *rgbdec);



JpegRGBDecoder *jpeg_rgb_decoder_new(void)
{
	JpegRGBDecoder *rgbdec;

	rgbdec = g_new0(JpegRGBDecoder, 1);

	rgbdec->dec = jpeg_decoder_new();

	return rgbdec;
}

int jpeg_rgb_decoder_addbits(JpegRGBDecoder *dec, unsigned char *data,
	unsigned int len)
{
	return jpeg_decoder_addbits(dec->dec, data, len);
}

int jpeg_rgb_decoder_parse(JpegRGBDecoder *dec)
{
	return jpeg_decoder_parse(dec->dec);
}

int jpeg_rgb_decoder_get_image(JpegRGBDecoder *rgbdec,
	unsigned char **image, int *rowstride, int *width, int *height)
{
	int i;

	rgbdec->width = 0;
	rgbdec->height = 0;
	for(i=0;i<3;i++){
		jpeg_decoder_get_component(rgbdec->dec, i+1,
			&rgbdec->component[i].image,
			&rgbdec->component[i].rowstride,
			&rgbdec->component[i].width,
			&rgbdec->component[i].height);
		rgbdec->width = MAX(rgbdec->width,
			rgbdec->component[i].width);
		rgbdec->height = MAX(rgbdec->height,
			rgbdec->component[i].height);
	}

	rgbdec->image = g_malloc(rgbdec->width * rgbdec->height * 4);

	convert(rgbdec);

	if(image) *image = rgbdec->image;
	if(rowstride) *rowstride = rgbdec->width * 4;
	if(width) *width = rgbdec->width;
	if(height) *height = rgbdec->height;

	return 0;
}

static void convert(JpegRGBDecoder *rgbdec)
{
	int x,y;
	unsigned char *yp;
	unsigned char *rgbp;

	rgbp = rgbdec->image;
	yp = rgbdec->component[0].image;
	for(y=0;y<rgbdec->height;y++){
		for(x=0;x<rgbdec->width;x++){
			rgbp[0] = yp[x];
			rgbp[1] = yp[x];
			rgbp[2] = yp[x];
			rgbp[3] = 0;
			rgbp+=4;
		}
		yp += rgbdec->component[0].rowstride;
	}
}

