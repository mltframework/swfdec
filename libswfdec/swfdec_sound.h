
#ifndef __SWFDEC_SOUND_H__
#define __SWFDEC_SOUND_H__

#include "swfdec_types.h"

struct swfdec_sound_struct{
	int format;

	unsigned char *orig_data;
	int orig_len;

#ifdef HAVE_MAD
	struct mad_stream stream;
	struct mad_frame frame;
	struct mad_synth synth;
	unsigned char tmpbuf[1024];
	int tmpbuflen;
#else
	void *mp;
#endif

	int n_samples;

	void *sound_buf;
	int sound_len;
};

struct swfdec_sound_buffer_struct{
	int len;
	unsigned char *data;
};

void swfdec_sound_free(SwfdecObject *object);
int tag_func_define_sound(SwfdecDecoder *s);
int tag_func_sound_stream_block(SwfdecDecoder *s);
int tag_func_sound_stream_head(SwfdecDecoder *s);
int tag_func_start_sound(SwfdecDecoder *s);
int tag_func_define_button_sound(SwfdecDecoder *s);

#endif

