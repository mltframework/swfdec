
#include <swfdec_sound.h>
#include <string.h>
#include <config.h>

#ifdef HAVE_MAD
#include <mad.h>
#endif

#include "swfdec_internal.h"


static void swfdec_sound_mp3_init (SwfdecSound * sound);
static void swfdec_sound_mp3_decode (SwfdecSound * sound);
int swfdec_sound_mp3_decode_stream (SwfdecDecoder * s, SwfdecSound * sound);
static void swfdec_sound_mp3_cleanup (SwfdecSound * sound);
#ifdef HAVE_MAD
static SwfdecBuffer * convert_synth_to_buffer (SwfdecSound *sound);
#endif

void adpcm_decode (SwfdecDecoder * s, SwfdecSound * sound);

SWFDEC_OBJECT_BOILERPLATE (SwfdecSound, swfdec_sound)

     static void swfdec_sound_base_init (gpointer g_class)
{

}

static void
swfdec_sound_class_init (SwfdecSoundClass * g_class)
{

}

static void
swfdec_sound_init (SwfdecSound * sound)
{

}

static void
swfdec_sound_dispose (SwfdecSound * sound)
{
  if (sound->format == 2) {
    swfdec_sound_mp3_cleanup (sound);
  }
  /* FIXME free uncompressed sound */

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (sound));
}


int
tag_func_sound_stream_block (SwfdecDecoder * s)
{
  SwfdecSound *sound;
  SwfdecBuffer *chunk;
  int n_samples;
  int n_left;

  /* for MPEG, data starts after 4 byte header */

  sound = SWFDEC_SOUND (s->stream_sound_obj);

  if (!sound) {
    SWFDEC_WARNING ("no streaming sound block");
    return SWF_OK;
  }

  if (sound->format != 2) {
    SWFDEC_WARNING ("tag_func_define_sound: unknown format %d", sound->format);
    return SWF_OK;
  }

  n_samples = swfdec_bits_get_u16 (&s->b);
  n_left = swfdec_bits_get_u16 (&s->b);
  if (s->b.ptr == s->b.end) {
    SWFDEC_DEBUG ("empty sound block n_samples=%d n_left=%d", n_samples,
        n_left);
    return SWF_OK;
  }

  chunk = swfdec_buffer_new_subbuffer (s->b.buffer, 4, s->b.buffer->length - 4);
  s->b.ptr += s->b.buffer->length - 4;

  swfdec_sprite_add_sound_chunk (s->parse_sprite, chunk,
      s->parse_sprite->parse_frame);

  return SWF_OK;
}


int
tag_func_define_sound (SwfdecDecoder * s)
{
  //static char *format_str[16] = { "uncompressed", "adpcm", "mpeg" };
  //static int rate_n[4] = { 5512.5, 11025, 22050, 44100 };
  SwfdecBits *b = &s->b;
  int id;
  int format;
  int rate;
  int size;
  int type;
  int n_samples;
  SwfdecSound *sound;
  int len;

  id = swfdec_bits_get_u16 (b);
  format = swfdec_bits_getbits (b, 4);
  rate = swfdec_bits_getbits (b, 2);
  size = swfdec_bits_getbits (b, 1);
  type = swfdec_bits_getbits (b, 1);
  n_samples = swfdec_bits_get_u32 (b);

  sound = swfdec_object_new (s, SWFDEC_TYPE_SOUND);
  SWFDEC_OBJECT (sound)->id = id;
  s->objects = g_list_append (s->objects, sound);

  sound->n_samples = n_samples;
  sound->format = format;

  switch (format) {
    case 2:
      /* unknown */
      len = swfdec_bits_get_u16 (b);

      sound->orig_buffer = swfdec_buffer_new_subbuffer (s->b.buffer, 9,
          s->b.buffer->length - 9);

      swfdec_sound_mp3_decode (sound);

      s->b.ptr += s->b.buffer->length - 9;
      break;
    case 1:
      //g_print("  size = %d (%d bit)\n", size, size ? 16 : 8);
      //g_print("  type = %d (%d channels)\n", type, type + 1);
      //g_print("  n_samples = %d\n", n_samples);
      adpcm_decode (s, sound);
      break;
    default:
      SWFDEC_WARNING ("tag_func_define_sound: unknown format %d", format);
  }

  return SWF_OK;
}

int
tag_func_sound_stream_head (SwfdecDecoder * s)
{
  //static char *format_str[16] = { "uncompressed", "adpcm", "mpeg" };
  //static int rate_n[4] = { 5512.5, 11025, 22050, 44100 };
  SwfdecBits *b = &s->b;
  int mix_format;
  int format;
  int rate;
  int size;
  int type;
  int n_samples;
  int latency;
  SwfdecSound *sound;

  mix_format = swfdec_bits_get_u8 (b);
  format = swfdec_bits_getbits (b, 4);
  rate = swfdec_bits_getbits (b, 2);
  size = swfdec_bits_getbits (b, 1);
  type = swfdec_bits_getbits (b, 1);
  n_samples = swfdec_bits_get_u16 (b);
  if (format == 2) {
    latency = swfdec_bits_get_u16 (b);
  }

  sound = swfdec_object_new (s, SWFDEC_TYPE_SOUND);
  SWFDEC_OBJECT (sound)->id = 0;
  s->objects = g_list_append (s->objects, sound);

  if (s->parse_sprite == s->main_sprite) {
    s->stream_sound_obj = sound;
  } else {
    SWFDEC_WARNING ("ignoring stream sound object for sprite");
  }

  sound->format = format;

  switch (format) {
    case 2:
      swfdec_sound_mp3_init (sound);

      break;
    default:
      SWFDEC_WARNING ("unimplemented sound format %d", format);
  }

  return SWF_OK;
}

static SwfdecSoundChunk *
get_soundinfo (SwfdecBits * b)
{
  int has_envelope;
  int has_loops;
  int has_out_point;
  int has_in_point;
  int envelope_n_points = 0;
  int mark44;
  int level0;
  int level1;
  int i;
  SwfdecSoundChunk *chunk;

  chunk = g_malloc0(sizeof(SwfdecSoundChunk));

  swfdec_bits_getbits (b, 2);
  chunk->stop = swfdec_bits_getbits (b, 1);
  chunk->no_restart = swfdec_bits_getbits (b, 1);
  has_envelope = swfdec_bits_getbits (b, 1);
  has_loops = swfdec_bits_getbits (b, 1);
  has_out_point = swfdec_bits_getbits (b, 1);
  has_in_point = swfdec_bits_getbits (b, 1);
  if (has_in_point) {
    chunk->start_sample = swfdec_bits_get_u32 (b);
  }
  if (has_out_point) {
    chunk->stop_sample = swfdec_bits_get_u32 (b);
  }
  if (has_loops) {
    chunk->loop_count = swfdec_bits_get_u16 (b);
  } else {
    chunk->loop_count = 1;
  }
  if (has_envelope) {
    envelope_n_points = swfdec_bits_get_u8 (b);
  }
  //g_print("  syncflags = %d\n", syncflags);
  //g_print("  has_envelope = %d\n", has_envelope);
  //g_print("  has_loops = %d\n", has_loops);
  //g_print("  has_out_point = %d\n", has_out_point);
  //g_print("  has_in_point = %d\n", has_in_point);
  //g_print("  in_point = %d\n", in_point);
  //g_print("  out_point = %d\n", out_point);
  //g_print("  loop_count = %d\n", loop_count);
  //g_print("  envelope_n_points = %d\n", envelope_n_points);

  for (i = 0; i < envelope_n_points; i++) {
    mark44 = swfdec_bits_get_u32 (b);
    level0 = swfdec_bits_get_u16 (b);
    level1 = swfdec_bits_get_u16 (b);

    //g_print("   mark44 = %d\n",mark44);
    //g_print("   level0 = %d\n",level0);
    //g_print("   level1 = %d\n",level1);
  }

  return chunk;
}

int
tag_func_start_sound (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;
  SwfdecSoundChunk *chunk;
  int id;

  id = swfdec_bits_get_u16 (b);

  SWFDEC_DEBUG ("start sound");

  chunk = get_soundinfo (b);
  chunk->object = id;

  s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_play = chunk;

  return SWF_OK;
}

int
tag_func_define_button_sound (SwfdecDecoder * s)
{
  int id;
  int i;
  int state;

  id = swfdec_bits_get_u16 (&s->b);
  //g_print("  id = %d\n",id);
  for (i = 0; i < 4; i++) {
    state = swfdec_bits_get_u16 (&s->b);
    //g_print("   state = %d\n",state);
    if (state) {
      get_soundinfo (&s->b);
    }
  }

  return SWF_OK;
}

static int index_adjust[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8,
};

static int step_size[89] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
  34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
  130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
  408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166,
  1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
  3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
  8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500,
  20350, 22385, 24623, 27086, 29794, 32767
};

void
adpcm_decode (SwfdecDecoder * s, SwfdecSound * sound)
{
  SwfdecBits *bits = &s->b;
  int n_bits;
  int sample;
  int index;
  int x;
  int i;
  int diff;
  int n, n_samples;
  int j = 0;

#if 0
  sample = swfdec_bits_get_u8 (bits) << 8;
  sample |= swfdec_bits_get_u8 (bits);
  g_print ("sample %d\n", sample);
#endif
  n_bits = swfdec_bits_getbits (bits, 2) + 2;
  //g_print("n_bits = %d\n",n_bits);

  if (n_bits != 4)
    return;

  n_samples = sound->n_samples;
  while (n_samples) {
    n = n_samples;
    if (n > 4096)
      n = 4096;

    sample = swfdec_bits_getsbits (bits, 16);
    //g_print("sample = %d\n",sample);
    index = swfdec_bits_getbits (bits, 6);
    //g_print("index = %d\n",index);


    for (i = 1; i < n; i++) {
      x = swfdec_bits_getbits (bits, n_bits);

      diff = (step_size[index] * (x & 0x7)) >> 2;
      diff += step_size[index] >> 3;
      if (x & 8)
        diff = -diff;

      sample += diff;

      if (sample < -32768)
        sample = -32768;
      if (sample > 32767)
        sample = 32767;

      index += index_adjust[x];
      if (index < 0)
        index = 0;
      if (index > 88)
        index = 88;

      j++;
    }
    n_samples -= n;
  }
}

#if 0
void
swfdec_sound_render (SwfdecDecoder * s)
{
  int len;
  GList *g;
  SwfdecBuffer *buffer;
  SwfdecBuffer *buf;
  int offset = 0;
  int n;

  len = 2 * 2 * (44100 / s->rate);
  buffer = swfdec_buffer_new_and_alloc (len);
  memset (buffer->data, 0, len);

  while (1) {
    if (!s->stream_sound_buffers)
      break;
    g = g_list_first (s->stream_sound_buffers);
    if (!g)
      break;

    buf = (SwfdecBuffer *) g->data;
    n = MIN (buf->length, len - offset);

    /* FIXME this isn't composing */
    memcpy (buffer->data + offset, buf->data, n);
    offset += n;

    if (n < buf->length) {
      SwfdecBuffer *subbuffer;

      subbuffer = swfdec_buffer_new_subbuffer (buf, n, buf->length - n);
      g->data = subbuffer;
      swfdec_buffer_unref (buf);
    } else {
      swfdec_buffer_unref (buf);
      s->stream_sound_buffers = g_list_delete_link (s->stream_sound_buffers, g);
    }

    if (offset >= len) {
      break;
    }
  }

  SWFDEC_LOG ("sound buffer: len=%d filled %d", len, offset);

  s->sound_buffers = g_list_append (s->sound_buffers, buffer);
}
#endif


#ifdef HAVE_MAD
static void
swfdec_sound_mp3_init (SwfdecSound * sound)
{
  mad_stream_init (&sound->stream);
  mad_frame_init (&sound->frame);
  mad_synth_init (&sound->synth);
}

static void
swfdec_sound_mp3_cleanup (SwfdecSound * sound)
{
  mad_synth_finish (&sound->synth);
  mad_frame_finish (&sound->frame);
  mad_stream_finish (&sound->stream);
}

void
swfdec_sound_mp3_decode (SwfdecSound * sound)
{
  int ret;
  SwfdecBuffer *buffer;

  mad_stream_init (&sound->stream);
  mad_frame_init (&sound->frame);
  mad_synth_init (&sound->synth);

  mad_stream_buffer (&sound->stream, sound->orig_buffer->data,
      sound->orig_buffer->length);

  while (1) {
    ret = mad_frame_decode (&sound->frame, &sound->stream);
    if (ret == -1 && sound->stream.error == MAD_ERROR_BUFLEN) {
      break;
    }
    if (ret == -1 && sound->stream.error == MAD_ERROR_LOSTSYNC) {
      mad_stream_sync (&sound->stream);
      continue;
    }
    if (ret == -1) {
      SWFDEC_ERROR ("stream error 0x%04x\n", sound->stream.error);
      break;
    }

    mad_synth_frame (&sound->synth, &sound->frame);
    buffer = convert_synth_to_buffer (sound);
    if (buffer) {
      sound->decoded_buffers = g_list_append (sound->decoded_buffers, buffer);
    }
  }

  mad_synth_finish (&sound->synth);
  mad_frame_finish (&sound->frame);
  mad_stream_finish (&sound->stream);
}

int
swfdec_sound_mp3_decode_stream (SwfdecDecoder * s, SwfdecSound * sound)
{
  int ret;

  mad_stream_buffer (&sound->stream, sound->tmpbuf, sound->tmpbuflen);

  while (sound->tmpbuflen >= 0) {
    SwfdecBuffer *buffer;

    ret = mad_frame_decode (&sound->frame, &sound->stream);
    if (ret == -1 && sound->stream.error == MAD_ERROR_BUFLEN) {
      break;
    }
    if (ret == -1 && sound->stream.error == MAD_ERROR_LOSTSYNC) {
      mad_stream_sync (&sound->stream);
      continue;
    }
    if (ret == -1) {
      SWFDEC_ERROR ("stream error 0x%04x", sound->stream.error);
      sound->tmpbuflen = 0;
      mad_stream_sync (&sound->stream);
      return SWF_ERROR;
    }

    mad_synth_frame (&sound->synth, &sound->frame);

    buffer = convert_synth_to_buffer (sound);
    if (buffer) {
      swfdec_audio_stream_push_buffer (s, 0, buffer);
    }
  }

  sound->tmpbuflen -= sound->stream.next_frame - sound->tmpbuf;
  memmove (sound->tmpbuf, sound->stream.next_frame, sound->tmpbuflen);

  return SWF_OK;
}

static SwfdecBuffer *
convert_synth_to_buffer (SwfdecSound *sound)
{
  SwfdecBuffer *buffer;
  int n_samples;
  short *data;
  int i;
  short c0,c1;

  n_samples = sound->synth.pcm.length;
  if (n_samples == 0) return NULL;

  switch (sound->synth.pcm.samplerate) {
    case 11025:
      n_samples *= 4;
      break;
    case 22050:
      n_samples *= 2;
      break;
    case 44100:
      break;
    default:
      SWFDEC_ERROR ("sample rate not handled (%d)",
          sound->synth.pcm.samplerate);
      return NULL;
  }

  buffer = swfdec_buffer_new_and_alloc (n_samples * 2 * 2);
  data = (short *) buffer->data;

  if (sound->synth.pcm.samplerate == 11025) {
    if (sound->synth.pcm.channels == 2) {
      for (i = 0; i < sound->synth.pcm.length; i++) {
        c0 = sound->synth.pcm.samples[0][i] >> 14;
        c1 = sound->synth.pcm.samples[1][i] >> 14;
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < sound->synth.pcm.length; i++) {
        c0 = sound->synth.pcm.samples[0][i] >> 14;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else if (sound->synth.pcm.samplerate == 22050) {
    if (sound->synth.pcm.channels == 2) {
      for (i = 0; i < sound->synth.pcm.length; i++) {
        c0 = sound->synth.pcm.samples[0][i] >> 14;
        c1 = sound->synth.pcm.samples[1][i] >> 14;
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < sound->synth.pcm.length; i++) {
        c0 = sound->synth.pcm.samples[0][i] >> 14;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else if (sound->synth.pcm.samplerate == 44100) {
    if (sound->synth.pcm.channels == 2) {
      for (i = 0; i < sound->synth.pcm.length; i++) {
        c0 = sound->synth.pcm.samples[0][i] >> 14;
        c1 = sound->synth.pcm.samples[1][i] >> 14;
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < sound->synth.pcm.length; i++) {
        c0 = sound->synth.pcm.samples[0][i] >> 14;
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else {
    SWFDEC_ERROR ("sample rate not handled (%d)",
        sound->synth.pcm.samplerate);
  }
  return buffer;
}
#else

static void
swfdec_sound_mp3_init (SwfdecSound * sound)
{

}

static void
swfdec_sound_mp3_decode (SwfdecSound * sound)
{
}

int
swfdec_sound_mp3_decode_stream (SwfdecDecoder * s, SwfdecSound * sound)
{
  return SWF_OK;
}

static void
swfdec_sound_mp3_cleanup (SwfdecSound * sound)
{
}

#endif
