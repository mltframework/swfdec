
#ifndef __SWFDEC_SOUND_H__
#define __SWFDEC_SOUND_H__

#include "swfdec_types.h"

struct swfdec_sound_struct{
	unsigned char *uncompressed_data;
	int format;

	unsigned char *orig_data;
	int orig_len;

	void *mp;

	int n_samples;

	void *sound_buf;
	int sound_len;
};

int tag_func_define_sound(SwfdecDecoder *s);
int tag_func_sound_stream_block(SwfdecDecoder *s);
int tag_func_sound_stream_head(SwfdecDecoder *s);
int tag_func_start_sound(SwfdecDecoder *s);
int tag_func_define_button_sound(SwfdecDecoder *s);

#endif

