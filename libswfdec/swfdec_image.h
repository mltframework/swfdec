
#ifndef __SWFDEC_IMAGE_H__
#define __SWFDEC_IMAGE_H__

struct swfdec_image_struct{
	int width, height;
	int rowstride;
	char *image_data;
};

int tag_func_define_bits_jpeg(SwfdecDecoder *s);
int tag_func_define_bits_jpeg_2(SwfdecDecoder *s);
int tag_func_define_bits_jpeg_3(SwfdecDecoder *s);
int tag_func_define_bits_lossless(SwfdecDecoder *s);
int tag_func_define_bits_lossless_2(SwfdecDecoder *s);
int swfdec_image_render(SwfdecDecoder *s, SwfdecLayer *layer);

#endif

