
#ifndef _JPEG_H_
#define _JPEG_H_

typedef struct jpeg_decoder_struct JpegDecoder;


JpegDecoder *jpeg_decoder_new(void);
int jpeg_decoder_addbits(JpegDecoder *dec, unsigned char *data, unsigned int len);
int jpeg_decoder_parse(JpegDecoder *dec);
char *jpeg_decoder_get_image(JpegDecoder *dec, int component);


#endif

