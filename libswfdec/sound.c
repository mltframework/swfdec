
#include <string.h>
#include <config.h>

#ifdef HAVE_MAD
#include <mad.h>
#else
#define getbits mpg_getbits
#include "mpglib/mpglib.h"
#undef getbits
#endif

#include "swfdec_internal.h"


void adpcm_decode(SwfdecDecoder *s,SwfdecObject *obj);
SwfdecSoundBuffer *swfdec_sound_buffer_new(int len);
void swfdec_decoder_sound_buffer_append(SwfdecDecoder *s, SwfdecSoundBuffer *buffer);


void swfdec_sound_free(SwfdecObject *object)
{
	SwfdecSound *sound = object->priv;

	if(sound->sound_buf)g_free(sound->sound_buf);

	g_free(sound);
}

#ifdef HAVE_MAD
void mp3_decode_mad(SwfdecObject *obj)
{
	struct mad_stream stream;
	struct mad_frame frame;
	struct mad_synth synth;
	SwfdecSound *sound = obj->priv;
	int ret;

	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);

	mad_stream_buffer(&stream, sound->orig_data, sound->orig_len);

	while(1){
		ret = mad_frame_decode(&frame, &stream);
		if(ret == -1 && stream.error == MAD_ERROR_BUFLEN){
			break;
		}
		if(ret == -1){
			printf("stream error 0x%04x\n",stream.error);
			return;
		}

		mad_synth_frame(&synth, &frame);
	}

	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
}

int tag_func_sound_stream_block(SwfdecDecoder *s)
{
	SwfdecObject *obj;
	SwfdecSound *sound;
	int ret;
	int n_samples, n_left;

	/* for MPEG, data starts after 4 byte header */

	obj = s->stream_sound_obj;
	sound = obj->priv;

	if(sound->format != 2){
		SWF_DEBUG(4,"tag_func_define_sound: unknown format %d\n",sound->format);
		return SWF_OK;
	}

	n_samples = get_u16(&s->b);
	n_left = get_u16(&s->b);
	//printf("sound stream %d %d %d\n", ack1, ack2, s->sound_offset/2);

	if(s->tag_len - 4 == 0){
		/* the end? */
		return SWF_OK;
	}

	memcpy(sound->tmpbuf + sound->tmpbuflen, s->b.ptr, s->tag_len - 4);
	sound->tmpbuflen += s->tag_len - 4;

	mad_stream_buffer(&sound->stream, sound->tmpbuf, sound->tmpbuflen);

	while(sound->tmpbuflen >= 0){
		ret = mad_frame_decode(&sound->frame, &sound->stream);
		if(ret == -1 && sound->stream.error == MAD_ERROR_BUFLEN){
			//fprintf(stderr,"error buflen\n");
			break;
		}
		if(ret == -1){
			fprintf(stderr,"stream error 0x%04x\n",
				sound->stream.error);
			return SWF_ERROR;
		}

		mad_synth_frame(&sound->synth, &sound->frame);
		
		if(sound->synth.pcm.samplerate==11025){
			SwfdecSoundBuffer *buffer;
			short *data;
			int i;

			buffer = swfdec_sound_buffer_new(sound->synth.pcm.length*2*2*4);
			data = (short *)buffer->data;
			if(sound->synth.pcm.channels==2){
				for(i=0;i<sound->synth.pcm.length;i++){
					short c0,c1;
					c0 = sound->synth.pcm.samples[0][i]>>13;
					c1 = sound->synth.pcm.samples[1][i]>>13;
					*data++ = c0; *data++ = c1;
					*data++ = c0; *data++ = c1;
					*data++ = c0; *data++ = c1;
					*data++ = c0; *data++ = c1;
				}
			}else{
				for(i=0;i<sound->synth.pcm.length;i++){
					short c0;
					c0 = sound->synth.pcm.samples[0][i]>>13;
					*data++ = c0; *data++ = c0;
					*data++ = c0; *data++ = c0;
					*data++ = c0; *data++ = c0;
					*data++ = c0; *data++ = c0;
				}
			}
			swfdec_decoder_sound_buffer_append(s,buffer);
		}else if(sound->synth.pcm.samplerate==22050){
			SwfdecSoundBuffer *buffer;
			short *data;
			int i;

			buffer = swfdec_sound_buffer_new(sound->synth.pcm.length*2*2*2);
			data = (short *)buffer->data;
			if(sound->synth.pcm.channels==2){
				for(i=0;i<sound->synth.pcm.length;i++){
					short c0,c1;
					c0 = sound->synth.pcm.samples[0][i]>>13;
					c1 = sound->synth.pcm.samples[1][i]>>13;
					*data++ = c0; *data++ = c1;
					*data++ = c0; *data++ = c1;
				}
			}else{
				for(i=0;i<sound->synth.pcm.length;i++){
					short c0;
					c0 = sound->synth.pcm.samples[0][i]>>13;
					*data++ = c0; *data++ = c0;
					*data++ = c0; *data++ = c0;
				}
			}
			swfdec_decoder_sound_buffer_append(s,buffer);
		}else{
			fprintf(stderr,"sample rate not handled (%d)\n",
				sound->synth.pcm.samplerate);
		}
	}

	sound->tmpbuflen -= sound->stream.next_frame - sound->tmpbuf;
	memmove(sound->tmpbuf, sound->stream.next_frame, sound->tmpbuflen);

	s->b.ptr += s->tag_len - 4;

	return SWF_OK;
}
#endif


#ifndef HAVE_MAD
void mp3_decode_mpglib(SwfdecObject *obj)
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
#endif

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

#ifdef HAVE_MAD
		mp3_decode_mad(obj);
#else
		mp3_decode_mpglib(obj);
#endif

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

void swfdec_decoder_sound_buffer_append(SwfdecDecoder *s, SwfdecSoundBuffer *buffer)
{
	s->stream_sound_buffers = g_list_append(s->stream_sound_buffers, buffer);
}

SwfdecSoundBuffer *swfdec_sound_buffer_new(int len)
{
	SwfdecSoundBuffer *sb;

	sb = g_new0(SwfdecSoundBuffer,1);

	sb->len = len;
	sb->offset = 0;
	sb->data = g_malloc(len);

	return sb;
}

#ifndef HAVE_MAD
int tag_func_sound_stream_block(SwfdecDecoder *s)
{
	SwfdecObject *obj;
	SwfdecSound *sound;
	SwfdecSoundBuffer *buffer;
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

	n_samples = get_u16(&s->b);
	n_left = get_u16(&s->b);
	//printf("sound stream %d %d %d\n", ack1, ack2, s->sound_offset/2);

	if(s->tag_len - 4 == 0){
		/* the end? */
		return SWF_OK;
	}

	buffer = swfdec_sound_buffer_new(4608);

	fwrite(s->b.ptr,s->tag_len - 4, 1, stdout);
	ret = mpglib_decoder_decode(sound->mp, s->b.ptr, s->tag_len - 4,
		buffer->data, buffer->len, &n);
	while(ret==MPGLIB_OK){
		int i;
		for(i=0;i<64;i++){
			fprintf(stderr,"%02x ",((unsigned char *)buffer->data)[i]);
			if((i%16)==15)fprintf(stderr,"\n");
		}
		swfdec_decoder_sound_buffer_append(s,buffer);

		buffer = swfdec_sound_buffer_new(4608);
		ret = mpglib_decoder_decode(sound->mp, NULL, 0,
			buffer->data, buffer->len, &n);
	}

	if(ret==MPGLIB_ERR){
		SWF_DEBUG(4,"mp3 stream error\n");
	}

	g_free(buffer->data);
	g_free(buffer);

	s->b.ptr += s->tag_len - 4;

	return SWF_OK;
}
#endif

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
#ifdef HAVE_MAD
		mad_stream_init(&sound->stream);
		mad_frame_init(&sound->frame);
		mad_synth_init(&sound->synth);
#else
		sound->mp = mpglib_decoder_new();
#endif
		
		break;
	default:
		SWF_DEBUG(4,"unimplemented sound format\n");
	}

	return SWF_OK;
}

static void get_soundinfo(bits_t *b)
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

void swfdec_sound_render(SwfdecDecoder *s)
{
	int len;
	GList *g;
	SwfdecSoundBuffer *buffer;
	SwfdecSoundBuffer *buf;
	int offset = 0;
	int n;

	len = 2*2*(44100/s->rate);
	buffer = swfdec_sound_buffer_new(len);
	memset(buffer->data,0,len);

	while(1){
		if(!s->stream_sound_buffers)break;
		g = g_list_first(s->stream_sound_buffers);
		if(!g)break;

		buf = (SwfdecSoundBuffer *)g->data;
		n = MIN(buf->len - buf->offset,len - offset);

		/* FIXME this isn't composing */
		memcpy(buffer->data + offset, buf->data + buf->offset,n);
		offset += n;
		buf->offset += n;

		if(buf->offset >= buf->len){
			g_free(buf->data);
			s->stream_sound_buffers = 
				g_list_delete_link(s->stream_sound_buffers,g);
		}

		if(offset >= len){
			break;
		}
	}

	s->sound_buffers = g_list_append(s->sound_buffers, buffer);
}

