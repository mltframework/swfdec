
#include <string.h>

#define getbits mpg_getbits
#include "mpglib/mpglib.h"
#undef getbits

#include "swfdec_internal.h"


void adpcm_decode(SwfdecDecoder *s,SwfdecObject *obj);

void mp3_decode(SwfdecObject *obj)
{
	MpglibDecoder *mp;
	int n;
	int offset = 0;
	int ret;
	SwfdecSound *sound = obj->priv;

	mp = mpglib_decoder_new();
	ret = mpglib_decoder_decode(mp, (void *)sound->orig_data,
		sound->orig_len, sound->sound_buf, sound->sound_len, &n);
	printf("decoding %d bytes\n",sound->orig_len);
	while(ret==MPGLIB_OK){
		offset += n;
		ret = mpglib_decoder_decode(mp, NULL, 0,
			sound->sound_buf,
			sound->sound_len, &n);
	}
	printf("total decoded %d\n",offset + n);
}

int tag_func_define_sound(SwfdecDecoder *s)
{
	//static char *format_str[16] = { "uncompressed", "adpcm", "mpeg" };
	//static int rate_n[4] = { 5512.5, 11025, 22050, 44100 };
	bits_t *b = &s->b;
	int id;
	int format;
	int rate;
	int size;
	int type;
	int n_samples;
	SwfdecObject *obj;
	SwfdecSound *sound;
	int len;

	id = get_u16(b);
	format = getbits(b,4);
	rate = getbits(b,2);
	size = getbits(b,1);
	type = getbits(b,1);
	n_samples = get_u32(b);

	obj = swfdec_object_new(s,id);
	sound = g_new0(SwfdecSound,1);
	obj->priv = sound;
	obj->type = SWF_OBJECT_SOUND;

	sound->n_samples = n_samples;
	sound->format = format;

	switch(format){
	case 2:
		/* unknown */
		len = get_u16(b);
		printf("len = %d\n",len);

		sound->orig_data = b->ptr;
		sound->orig_len = s->tag_len - 9;

		sound->sound_len = 10000;
		sound->sound_buf = malloc(sound->sound_len);

		mp3_decode(obj);

		s->b.ptr += s->tag_len - 9;
		break;
	case 1:
		//printf("  size = %d (%d bit)\n", size, size ? 16 : 8);
		//printf("  type = %d (%d channels)\n", type, type + 1);
		//printf("  n_samples = %d\n", n_samples);
		adpcm_decode(s,obj);
		break;
	default:
		SWF_DEBUG(4,"tag_func_define_sound: unknown format %d\n",format);
	}

	return SWF_OK;
}

int tag_func_sound_stream_block(SwfdecDecoder *s)
{
	SwfdecObject *obj;
	SwfdecSound *sound;
	int ret;
	int n;
	int n_samples, n_left;

	/* for MPEG, data starts after 4 byte header */

	obj = s->stream_sound_obj;
	sound = obj->priv;

	if(sound->format != 2){
		SWF_DEBUG(4,"tag_func_define_sound: unknown format %d\n",sound->format);
		return SWF_OK;
	}

	if(s->sound_offset){
		int chunk;

		chunk = 44100.0/s->rate;
		chunk *= 2;
		if(chunk>s->sound_offset)chunk=s->sound_offset;

		memmove(s->sound_buffer,s->sound_buffer + chunk*2,
			s->sound_len - chunk*2);
		s->sound_offset -= chunk;
	}

	n_samples = get_u16(&s->b);
	n_left = get_u16(&s->b);
	//printf("sound stream %d %d %d\n", ack1, ack2, s->sound_offset/2);

	if(s->tag_len - 4 == 0){
		/* the end? */
		return SWF_OK;
	}

	ret = mpglib_decoder_decode(sound->mp, s->b.ptr, s->tag_len - 4,
		s->sound_buffer + s->sound_offset,
		s->sound_len - s->sound_offset, &n);
	while(ret==MPGLIB_OK){
		s->sound_offset += n;
		ret = mpglib_decoder_decode(sound->mp, NULL, 0,
			s->sound_buffer + s->sound_offset,
			s->sound_len - s->sound_offset, &n);
	}

	s->b.ptr += s->tag_len - 4;

	return SWF_OK;
}

int tag_func_sound_stream_head(SwfdecDecoder *s)
{
	//static char *format_str[16] = { "uncompressed", "adpcm", "mpeg" };
	//static int rate_n[4] = { 5512.5, 11025, 22050, 44100 };
	bits_t *b = &s->b;
	int mix_format;
	int format;
	int rate;
	int size;
	int type;
	int n_samples;
	int unknown;
	SwfdecObject *obj;
	SwfdecSound *sound;

	mix_format = get_u8(b);
	format = getbits(b,4);
	rate = getbits(b,2);
	size = getbits(b,1);
	type = getbits(b,1);
	n_samples = get_u16(b);
	unknown = get_u16(b);

	//printf("  mix_format = %d\n", mix_format);
	//printf("  format = %d (%s)\n", format, format_str[format]);
	//printf("  rate = %d (%d Hz) [spec: reserved]\n", rate, rate_n[rate]);
	//printf("  size = %d (%d bit)\n", size, size ? 16 : 8);
	//printf("  type = %d (%d channels)\n", type, type + 1);
	//printf("  n_samples = %d\n", n_samples); /* XXX per frame? */
	//printf("  unknown = %d\n", unknown);

	obj = swfdec_object_new(s,0);
	s->stream_sound_obj = obj;
	sound = g_new0(SwfdecSound,1);
	obj->priv = sound;
	obj->type = SWF_OBJECT_SOUND;
	sound->format = format;

	switch(format){
	case 2:
		sound->mp = mpglib_decoder_new();
		
		break;
	default:
		SWF_DEBUG(4,"unimplemented sound format\n");
	}

	return SWF_OK;
}

void get_soundinfo(bits_t *b)
{
	int syncflags;
	int has_envelope;
	int has_loops;
	int has_out_point;
	int has_in_point;
	int in_point = 0;
	int out_point = 0;
	int loop_count = 0;
	int envelope_n_points = 0;
	int mark44;
	int level0;
	int level1;
	int i;

	syncflags = getbits(b,4);
	has_envelope = getbits(b,1);
	has_loops = getbits(b,1);
	has_out_point = getbits(b,1);
	has_in_point = getbits(b,1);
	if(has_in_point){
		in_point = get_u32(b);
	}
	if(has_out_point){
		out_point = get_u32(b);
	}
	if(has_loops){
		loop_count = get_u16(b);
	}
	if(has_envelope){
		envelope_n_points = get_u8(b);
	}

	//printf("  syncflags = %d\n", syncflags);
	//printf("  has_envelope = %d\n", has_envelope);
	//printf("  has_loops = %d\n", has_loops);
	//printf("  has_out_point = %d\n", has_out_point);
	//printf("  has_in_point = %d\n", has_in_point);
	//printf("  in_point = %d\n", in_point);
	//printf("  out_point = %d\n", out_point);
	//printf("  loop_count = %d\n", loop_count);
	//printf("  envelope_n_points = %d\n", envelope_n_points);

	for(i=0;i<envelope_n_points;i++){
		mark44 = get_u32(b);
		level0 = get_u16(b);
		level1 = get_u16(b);

		//printf("   mark44 = %d\n",mark44);
		//printf("   level0 = %d\n",level0);
		//printf("   level1 = %d\n",level1);
	}

}

int tag_func_start_sound(SwfdecDecoder *s)
{
	bits_t *b = &s->b;
	int id;

	id = get_u16(b);

	//printf("  id = %d\n", id);
	get_soundinfo(b);

	return SWF_OK;
}

int tag_func_define_button_sound(SwfdecDecoder *s)
{
	int id;
	int i;
	int state;

	id = get_u16(&s->b);
	//printf("  id = %d\n",id);
	for(i=0;i<4;i++){
		state = get_u16(&s->b);
		//printf("   state = %d\n",state);
		if(state){
			get_soundinfo(&s->b);
		}
	}

	return SWF_OK;
}

int index_adjust[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8,
};

int step_size[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
	408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166,
	1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
	8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
	20350, 22385, 24623, 27086, 29794, 32767
};

void adpcm_decode(SwfdecDecoder *s,SwfdecObject *obj)
{
	bits_t *bits = &s->b;
	SwfdecSound *sound = obj->priv;
	int n_bits;
	int sample;
	int index;
	int x;
	int i;
	int diff;
	int n, n_samples;
	int j = 0;

#if 0
	sample = get_u8(bits)<<8;
	sample |= get_u8(bits);
	printf("sample %d\n",sample);
#endif
	n_bits = getbits(bits,2) + 2;
	//printf("n_bits = %d\n",n_bits);

	if(n_bits!=4)return;

	n_samples = sound->n_samples;
	while(n_samples){
		n = n_samples;
		if(n>4096)n=4096;

		sample = getsbits(bits,16);
		//printf("sample = %d\n",sample);
		index = getbits(bits,6);
		//printf("index = %d\n",index);


		for(i=1;i<n;i++){
			x = getbits(bits,n_bits);
	
			diff = (step_size[index]*(x&0x7))>>2;
			diff += step_size[index]>>3;
			if(x&8)diff=-diff;
	
			sample += diff;
	
			if(sample<-32768)sample=-32768;
			if(sample>32767)sample=32767;

			index += index_adjust[x];
			if(index<0)index=0;
			if(index>88)index=88;

			j++;
		}
		n_samples -= n;
	}
}

