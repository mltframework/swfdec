
#ifndef _JPEG_DECODER_H_
#define _JPEG_DECODER_H_

typedef struct jpeg_decoder_struct JpegDecoder;


JpegDecoder *jpeg_decoder_new(void);
int jpeg_decoder_addbits(JpegDecoder *dec, unsigned char *data, unsigned int len);
int jpeg_decoder_parse(JpegDecoder *dec);
int jpeg_decoder_get_component(JpegDecoder *dec, int id,
	unsigned char **image, int *rowstride, int *width, int *height);


#endif

