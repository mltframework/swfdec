
#ifndef _JPEG_DECODER_H_
#define _JPEG_DECODER_H_

typedef struct jpeg_decoder_struct JpegDecoder;


JpegDecoder *jpeg_decoder_new(void);
void jpeg_decoder_free(JpegDecoder *dec);
int jpeg_decoder_addbits(JpegDecoder *dec, unsigned char *data, unsigned int len);
int jpeg_decoder_parse(JpegDecoder *dec);
int jpeg_decoder_get_image_size(JpegDecoder *dec, int *width, int *height);
int jpeg_decoder_get_component_size(JpegDecoder *dec, int id,
	int *width, int *height);
int jpeg_decoder_get_component_subsampling(JpegDecoder *dec, int id,
	int *h_subsample, int *v_subsample);
int jpeg_decoder_get_component_ptr(JpegDecoder *dec, int id,
	unsigned char **image, int *rowstride);


#endif

