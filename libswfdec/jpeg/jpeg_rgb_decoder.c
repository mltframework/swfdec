
#include <glib.h>

#include "jpeg_rgb_internal.h"
#include "jpeg.h"


static void convert(JpegRGBDecoder *rgbdec);
static void upscale_y(JpegRGBDecoder *rgbdec, int i);
static void upscale_x(JpegRGBDecoder *rgbdec, int i);



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
		if(i>0){
			rgbdec->component[i].width /= 2;
			rgbdec->component[i].height /= 2;
		}
		rgbdec->width = MAX(rgbdec->width,
			rgbdec->component[i].width);
		rgbdec->height = MAX(rgbdec->height,
			rgbdec->component[i].height);
	}

	rgbdec->image = g_malloc(rgbdec->width * rgbdec->height * 4);

	upscale_x(rgbdec,1);
	upscale_x(rgbdec,2);
	upscale_y(rgbdec,1);
	upscale_y(rgbdec,2);

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
	unsigned char *up;
	unsigned char *vp;
	unsigned char *rgbp;

	rgbp = rgbdec->image;
	yp = rgbdec->component[0].image;
	up = rgbdec->component[1].image;
	vp = rgbdec->component[2].image;
	for(y=0;y<rgbdec->height;y++){
		for(x=0;x<rgbdec->width;x++){
#if 1
			rgbp[0] = yp[x] + 1.402*(vp[x]-128);
			rgbp[1] = yp[x] - 0.34414*(up[x]-128) - 0.71414*(vp[x]-128);
			rgbp[2] = yp[x] + 1.772*(up[x]-128);
#else
			rgbp[0] = yp[x];
			rgbp[1] = yp[x];
			rgbp[2] = yp[x];
#endif
			rgbp[3] = 0;
			rgbp+=4;
		}
		yp += rgbdec->component[0].rowstride;
		up += rgbdec->component[1].rowstride;
		vp += rgbdec->component[2].rowstride;
	}
}

static void upscale_y(JpegRGBDecoder *rgbdec, int i)
{
	unsigned char *new_image;
	int new_height;
	int new_width;
	int y;

	new_height = rgbdec->component[i].height * 2;
	new_width = rgbdec->component[i].width;
	new_image = g_malloc(new_height * new_width);

	for(y=0;y<rgbdec->component[i].height; y++){
		memcpy(new_image + new_width*y*2,
			rgbdec->component[i].image +
			rgbdec->component[i].rowstride * y,
			new_width);
		memcpy(new_image + new_width*(y*2 + 1),
			rgbdec->component[i].image +
			rgbdec->component[i].rowstride * y,
			new_width);
	}

	rgbdec->component[i].image = new_image;
	rgbdec->component[i].width = new_width;
	rgbdec->component[i].height = new_height;
	rgbdec->component[i].rowstride = new_width;
}

static void upscale_x(JpegRGBDecoder *rgbdec, int i)
{
	unsigned char *new_image;
	int new_height;
	int new_width;
	int x,y;
	unsigned char *inp, *outp;

	new_height = rgbdec->component[i].height;
	new_width = rgbdec->component[i].width * 2;
	new_image = g_malloc(new_height * new_width);

	outp = new_image;
	inp = rgbdec->component[i].image;
	for(y=0;y<rgbdec->component[i].height; y++){
		for(x=0;x<rgbdec->component[i].width;x++){
			outp[x*2] = inp[x];
			outp[x*2+1] = inp[x];
		}
		outp += new_width;
		inp += rgbdec->component[i].rowstride;
	}

	rgbdec->component[i].image = new_image;
	rgbdec->component[i].width = new_width;
	rgbdec->component[i].height = new_height;
	rgbdec->component[i].rowstride = new_width;
}

