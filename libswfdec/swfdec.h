
#ifndef __SWFDEC_H__
#define __SWFDEC_H__

#include <glib.h>

G_BEGIN_DECLS

enum {
	SWF_OK = 0,
	SWF_NEEDBITS,
	SWF_WAIT,
	SWF_ERROR,
	SWF_EOF,
	SWF_IMAGE,
	SWF_CHANGE,
};

enum {
	SWF_COLORSPACE_RGB888 = 0,
	SWF_COLORSPACE_RGB565,
};

typedef struct swfdec_decoder_struct SwfdecDecoder;

/* deprecated  */
SwfdecDecoder *swf_init(void);

SwfdecDecoder *swfdec_decoder_new(void);
int swf_addbits(SwfdecDecoder *s, unsigned char *bits, int len);
int swf_parse(SwfdecDecoder *s);
int swf_parse_header(SwfdecDecoder *s);
int swf_parse_tag(SwfdecDecoder *s);

G_END_DECLS

#endif

