
#ifndef __SWF_H__
#define __SWF_H__

#include <swfdec.h>

/* deprecated  */

typedef struct swfdec_decoder_struct swf_state_t;

struct swfdec_decoder_struct {
	int version;
	int length;
	int width, height;
	double rate;
	int n_frames;
	char *buffer;
	int frame_number;

	/* other stuff */
};

SwfdecDecoder *swf_init(void);
int swf_addbits(SwfdecDecoder *s, unsigned char *bits, int len);
int swf_parse(SwfdecDecoder *s);

#endif

