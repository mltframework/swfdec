#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <config.h>
#include <liboil/liboil.h>
#ifdef HAVE_MAD
#include <mad.h>
#endif

#include "swfdec.h"
#include "swfdec_sound.h"
#include "swfdec_bits.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_sprite.h"

gpointer swfdec_sound_init_decoder (SwfdecSound * sound);

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
  if (sound->decoded)
    swfdec_buffer_unref (sound->decoded);

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (sound));
}

int
tag_func_sound_stream_block (SwfdecDecoder * s)
{
  SwfdecSound *sound;
  SwfdecBuffer *chunk;
  int n_samples;
  int skip;

  /* for MPEG, data starts after 4 byte header */

  sound = SWFDEC_SOUND (s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head);

  if (!sound) {
    SWFDEC_WARNING ("no streaming sound block");
    return SWFDEC_OK;
  }

  n_samples = swfdec_bits_get_u16 (&s->b);
  if (sound->format == SWFDEC_SOUND_FORMAT_MP3) {
    skip = swfdec_bits_get_s16 (&s->b);
  } else {
    skip = 0;
  }
  if (s->b.ptr == s->b.end) {
    SWFDEC_DEBUG ("empty sound block n_samples=%d skip=%d", n_samples,
        skip);
    return SWFDEC_OK;
  }

  chunk = swfdec_bits_get_buffer (&s->b, -1);
  if (chunk == NULL) {
    SWFDEC_ERROR ("empty sound chunk");
    return SWFDEC_OK;
  }
  SWFDEC_LOG ("got a buffer with %u samples, %d skip and %u bytes mp3 data", n_samples, skip,
      chunk->length);
  /* use this to write out the stream data to stdout - nice way to get an mp3 file :) */
  //write (1, (void *) chunk->data, chunk->length);

  swfdec_sprite_add_sound_chunk (s->parse_sprite, s->parse_sprite->parse_frame, chunk, skip);

  return SWFDEC_OK;
}

int
tag_func_define_sound (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;
  int id;
  int format;
  int rate;
  int size;
  int type;
  int n_samples;
  SwfdecSound *sound;
  unsigned int skip = 0;
  SwfdecBuffer *orig_buffer = NULL;

  id = swfdec_bits_get_u16 (b);
  format = swfdec_bits_getbits (b, 4);
  rate = swfdec_bits_getbits (b, 2);
  size = swfdec_bits_getbits (b, 1);
  type = swfdec_bits_getbits (b, 1);
  n_samples = swfdec_bits_get_u32 (b);

  sound = swfdec_object_create (s, id, SWFDEC_TYPE_SOUND);
  if (!sound)
    return SWFDEC_OK;

  sound->n_samples = n_samples;
  sound->width = size;
  sound->channels = type ? 2 : 1;
  sound->rate_multiplier = (1 << rate);

  switch (format) {
    case 0:
      if (size == 1)
	SWFDEC_WARNING ("undefined endianness for s16 sound");
      /* just assume LE and hope it works (FIXME: want a switch for this?) */
      /* fall through */
    case 3:
      sound->format = SWFDEC_SOUND_FORMAT_UNCOMPRESSED;
      orig_buffer = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 2:
      sound->format = SWFDEC_SOUND_FORMAT_MP3;
      /* FIXME: skip these samples */
      skip = swfdec_bits_get_u16 (b);
      orig_buffer = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 1:
      sound->format = SWFDEC_SOUND_FORMAT_ADPCM;
      //g_print("  size = %d (%d bit)\n", size, size ? 16 : 8);
      //g_print("  type = %d (%d channels)\n", type, type + 1);
      //g_print("  n_samples = %d\n", n_samples);
      //adpcm_decode (s, sound);
      SWFDEC_WARNING ("adpcm decoding doesn't work");
      break;
    case 6:
      sound->format = SWFDEC_SOUND_FORMAT_NELLYMOSER;
      SWFDEC_WARNING ("Nellymosere compression not implemented");
      break;
    default:
      SWFDEC_WARNING ("unknown format %d", format);
      sound->format = SWFDEC_SOUND_FORMAT_UNDEFINED;
  }
  if (orig_buffer) {
    SwfdecBuffer *tmp;
    gpointer data = swfdec_sound_init_decoder (sound);
    tmp = swfdec_sound_decode_buffer (sound, data, orig_buffer);
    swfdec_sound_finish_decoder (sound, data);
    swfdec_buffer_unref (orig_buffer);
    /* only assign here, the decoding code checks this */
    sound->decoded = tmp;
  }
  if (sound->decoded) {
    if (sound->decoded->length < sound->n_samples * 4) {
      SWFDEC_WARNING ("%d sound samples should be available, but only %d are",
	  sound->n_samples, sound->decoded->length / 4);
      sound->n_samples = sound->decoded->length / 4;
    } else {
      SWFDEC_LOG ("%d samples decoded", sound->n_samples);
    }
  } else {
    SWFDEC_ERROR ("defective sound object (id %d)", SWFDEC_OBJECT (sound)->id);
    s->characters = g_list_remove (s->characters, sound);
    g_object_unref (sound);
  }

  return SWFDEC_OK;
}

int
tag_func_sound_stream_head (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;
  int format;
  int rate;
  int size;
  int type;
  int n_samples;
  int latency;
  SwfdecSound *sound;

  /* we don't care about playback suggestions */
  swfdec_bits_getbits (b, 8);

  format = swfdec_bits_getbits (b, 4);
  rate = swfdec_bits_getbits (b, 2);
  size = swfdec_bits_getbits (b, 1);
  type = swfdec_bits_getbits (b, 1);
  n_samples = swfdec_bits_get_u16 (b);

  sound = swfdec_object_new (s, SWFDEC_TYPE_SOUND);

  if (s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head)
    g_object_unref (s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head);
  s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head = sound;

  sound->width = size;
  sound->channels = type ? 2 : 1;
  sound->rate_multiplier = (1 << rate);

  switch (format) {
    case 0:
      if (size == 1)
	SWFDEC_WARNING ("undefined endianness for s16 sound");
      /* just assume LE and hope it works (FIXME: want a switch for this?) */
      /* fall through */
    case 3:
      sound->format = SWFDEC_SOUND_FORMAT_UNCOMPRESSED;
      break;
    case 2:
      sound->format = SWFDEC_SOUND_FORMAT_MP3;
      /* latency seek */
      latency = swfdec_bits_get_s16 (b);
      break;
    case 1:
      sound->format = SWFDEC_SOUND_FORMAT_ADPCM;
      SWFDEC_WARNING ("adpcm decoding doesn't work");
      break;
    case 6:
      sound->format = SWFDEC_SOUND_FORMAT_NELLYMOSER;
      SWFDEC_WARNING ("Nellymosere compression not implemented");
      break;
    default:
      SWFDEC_WARNING ("unknown format %d", format);
      sound->format = SWFDEC_SOUND_FORMAT_UNDEFINED;
  }

  return SWFDEC_OK;
}

void
swfdec_sound_chunk_free (SwfdecSoundChunk *chunk)
{
  g_return_if_fail (chunk != NULL);

  g_free (chunk->envelope);
  g_free (chunk);
}

SwfdecSoundChunk *
swfdec_sound_parse_chunk (SwfdecDecoder *s, int id)
{
  int has_envelope;
  int has_loops;
  int has_out_point;
  int has_in_point;
  unsigned int i;
  SwfdecSound *sound;
  SwfdecSoundChunk *chunk;
  SwfdecBits *b = &s->b;

  sound = swfdec_object_get (s, id);
  if (!SWFDEC_IS_SOUND (sound)) {
    SWFDEC_ERROR ("given id %d does not reference a sound object", id);
    return NULL;
  }

  chunk = g_new0 (SwfdecSoundChunk, 1);
  chunk->sound = sound;
  g_print ("parsing sound chunk for sound %d\n", SWFDEC_OBJECT (sound)->id);

  swfdec_bits_getbits (b, 2);
  chunk->stop = swfdec_bits_getbits (b, 1);
  chunk->no_restart = swfdec_bits_getbits (b, 1);
  has_envelope = swfdec_bits_getbits (b, 1);
  has_loops = swfdec_bits_getbits (b, 1);
  has_out_point = swfdec_bits_getbits (b, 1);
  has_in_point = swfdec_bits_getbits (b, 1);
  SWFDEC_DEBUG ("Soundinfo");
  if (has_in_point) {
    chunk->start_sample = swfdec_bits_get_u32 (b);
  } else {
    chunk->start_sample = 0;
  }
  if (has_out_point) {
    chunk->stop_sample = swfdec_bits_get_u32 (b);
    if (chunk->stop_sample > sound->n_samples) {
      SWFDEC_WARNING ("more samples specified (%u) than available (%u)", 
	  chunk->stop_sample, sound->n_samples);
      chunk->stop_sample = sound->n_samples;
    }
  } else {
    chunk->stop_sample = sound->n_samples;
  }
  if (has_loops) {
    chunk->loop_count = swfdec_bits_get_u16 (b);
  } else {
    chunk->loop_count = 1;
  }
  if (has_envelope) {
    chunk->n_envelopes = swfdec_bits_get_u8 (b);
    chunk->envelope = g_new (SwfdecSoundEnvelope, chunk->n_envelopes);
  }
  SWFDEC_LOG ("  in_point = %d\n", chunk->start_sample);
  SWFDEC_LOG ("  out_point = %d\n", chunk->stop_sample);
  SWFDEC_LOG ("  loop_count = %d\n", chunk->loop_count);
  SWFDEC_LOG ("  n_envelopes = %d\n", chunk->n_envelopes);

  for (i = 0; i < chunk->n_envelopes; i++) {
    chunk->envelope[i].offset = swfdec_bits_get_u32 (b);
    if (chunk->envelope[i].offset < chunk->start_sample) {
      SWFDEC_WARNING ("envelope entry offset too small (%d vs %d)",
	  chunk->envelope[i].offset, chunk->start_sample);
      chunk->envelope[i].offset = chunk->start_sample;
    }
    if (chunk->envelope[i].offset >= chunk->stop_sample) {
      SWFDEC_WARNING ("envelope entry offset too big (%d vs %d)",
	  chunk->envelope[i].offset, chunk->stop_sample);
      chunk->envelope[i].offset = chunk->stop_sample - 1;
    }
    chunk->envelope[i].volume[0] = swfdec_bits_get_u16 (b);
    chunk->envelope[i].volume[1] = swfdec_bits_get_u16 (b);
  }

  return chunk;
}

int
tag_func_start_sound (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;
  SwfdecSoundChunk *chunk;
  int id;
  SwfdecSpriteFrame *frame = &s->parse_sprite->frames[s->parse_sprite->parse_frame];

  id = swfdec_bits_get_u16 (b);

  SWFDEC_DEBUG ("start sound");

  chunk = swfdec_sound_parse_chunk (s, id);
  if (chunk)
    frame->sound = g_slist_prepend (frame->sound, chunk);

  return SWFDEC_OK;
}

int
tag_func_define_button_sound (SwfdecDecoder * s)
{
  int id;
  int i;
  int state;
  SwfdecSoundChunk *chunk;

  id = swfdec_bits_get_u16 (&s->b);
  //g_print("  id = %d\n",id);
  for (i = 0; i < 4; i++) {
    state = swfdec_bits_get_u16 (&s->b);
    //g_print("   state = %d\n",state);
    if (state) {
      chunk = swfdec_sound_parse_chunk (s, id);
      SWFDEC_ERROR ("button sounds not implemented");
      if (chunk)
	swfdec_sound_chunk_free (chunk);
    }
  }

  return SWFDEC_OK;
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

#ifdef HAVE_MAD
typedef struct {
  struct mad_stream	stream;
  struct mad_frame	frame;
  struct mad_synth	synth;
  guint8		data[MAD_BUFFER_MDLEN * 3];
  guint			data_len;
} MadData;

static gpointer
swfdec_sound_mp3_init (SwfdecSound * sound)
{
  MadData *data = g_new (MadData, 1);

  mad_stream_init (&data->stream);
  mad_frame_init (&data->frame);
  mad_synth_init (&data->synth);
  data->data_len = 0;

  return data;
}

static void
swfdec_sound_mp3_cleanup (SwfdecSound *sound, gpointer datap)
{
  MadData *data = datap;

  mad_synth_finish (&data->synth);
  mad_frame_finish (&data->frame);
  mad_stream_finish (&data->stream);

  g_free (data);
}

static SwfdecBuffer *
convert_synth_to_buffer (SwfdecSound *sound, MadData *mdata)
{
  SwfdecBuffer *buffer;
  int n_samples;
  short *data;
  int i;
  short c0,c1;
#define MAD_F_TO_S16(x) (CLAMP (x, -MAD_F_ONE, MAD_F_ONE) >> (MAD_F_FRACBITS - 14))

  n_samples = mdata->synth.pcm.length;
  if (n_samples == 0) {
    return NULL;
  }

  switch (mdata->synth.pcm.samplerate) {
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
          mdata->synth.pcm.samplerate);
      return NULL;
  }

  buffer = swfdec_buffer_new_and_alloc (n_samples * 2 * 2);
  data = (gint16 *) buffer->data;

  if (mdata->synth.pcm.samplerate == 11025) {
    if (mdata->synth.pcm.channels == 2) {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        c1 = MAD_F_TO_S16 (mdata->synth.pcm.samples[1][i]);
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
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16( mdata->synth.pcm.samples[0][i]);
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
  } else if (mdata->synth.pcm.samplerate == 22050) {
    if (mdata->synth.pcm.channels == 2) {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        c1 = MAD_F_TO_S16 (mdata->synth.pcm.samples[1][i]);
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else if (mdata->synth.pcm.samplerate == 44100) {
    if (mdata->synth.pcm.channels == 2) {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        c1 = MAD_F_TO_S16 (mdata->synth.pcm.samples[1][i]);
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else {
    SWFDEC_ERROR ("sample rate not handled (%d)",
        mdata->synth.pcm.samplerate);
  }
  return buffer;
}

SwfdecBuffer *
swfdec_sound_mp3_decode (SwfdecSound * sound, gpointer datap, SwfdecBuffer *buffer)
{
  MadData *data = datap;
  SwfdecBuffer *out;
  SwfdecBufferQueue *queue;
  unsigned int amount = 0, size;

  queue = swfdec_buffer_queue_new ();

  //write (1, buffer->data, buffer->length);
  //g_print ("buffer %p gave us %u bytes\n", buffer, buffer->length);
  while (amount < buffer->length) {
    size = MIN (buffer->length - amount, MAD_BUFFER_MDLEN * 3 - data->data_len);
    memcpy (&data->data[data->data_len], buffer->data + amount, size);
    //write (1, buffer->data + amount, size);
    amount += size;
    data->data_len += size;
    mad_stream_buffer (&data->stream, data->data, data->data_len);
    while (1) {
      if (mad_frame_decode (&data->frame, &data->stream)) {
	if (data->stream.error == MAD_ERROR_BUFLEN)
	  break;
	if (MAD_RECOVERABLE (data->stream.error)) {
	  SWFDEC_LOG ("recoverable error 0x%04x", data->stream.error);
	  continue;
	}
	SWFDEC_ERROR ("stream error 0x%04x", data->stream.error);
	break;
      }

      mad_synth_frame (&data->synth, &data->frame);
      out = convert_synth_to_buffer (sound, data);
      if (out) {
	swfdec_buffer_queue_push (queue, out);
      }
    }
    if (data->stream.next_frame == NULL) {
      data->data_len = 0;
    } else {
      data->data_len = data->stream.bufend - data->stream.next_frame;
      memmove (data->data, data->stream.next_frame, data->data_len);
    }
  }
  //g_print ("%u bytes left\n", data->data_len);

  g_assert (sound->decoded == NULL);
  size = swfdec_buffer_queue_get_depth (queue);
  if (size > 0)
    out = swfdec_buffer_queue_pull (queue, size);
  else
    out = NULL;
  swfdec_buffer_queue_free (queue);

  return out;
}
#endif

gpointer 
swfdec_sound_init_decoder (SwfdecSound * sound)
{
  g_assert (sound->decoded == NULL);

#ifdef HAVE_MAD
  if (sound->format == SWFDEC_SOUND_FORMAT_MP3)
    return swfdec_sound_mp3_init (sound);
#endif
  
  return NULL;
}

void
swfdec_sound_finish_decoder (SwfdecSound * sound, gpointer data)
{
  g_assert (sound->decoded == NULL);

#ifdef HAVE_MAD
  if (sound->format == SWFDEC_SOUND_FORMAT_MP3)
    return swfdec_sound_mp3_cleanup (sound, data);
#endif
}

/* NB: this function can return NULL even in case of success because of internal buffering */
SwfdecBuffer *
swfdec_sound_decode_buffer (SwfdecSound *sound, gpointer data, SwfdecBuffer *buffer)
{
  SwfdecBuffer *ret;

  g_assert (sound->decoded == NULL);

  switch (sound->format) {
    case SWFDEC_SOUND_FORMAT_MP3:
#ifdef HAVE_MAD
      ret = swfdec_sound_mp3_decode (sound, data, buffer);
#endif
      break;
    case SWFDEC_SOUND_FORMAT_UNCOMPRESSED:
      ret = swfdec_buffer_new_and_alloc (buffer->length);
      g_assert (sound->width == TRUE); /* FIXME: handle 8bit unsigned */
#if G_BYTE_ORDER == G_BIG_ENDIAN
      oil_swab_u16 ((guint16 *) ret->data, (guint16 *) buffer->data, buffer->length / 2);
#else
      memcpy (ret->data, buffer->data, buffer->length);
#endif
      break;
    /* these should never happen */
    case SWFDEC_SOUND_FORMAT_ADPCM:
    case SWFDEC_SOUND_FORMAT_NELLYMOSER:
    case SWFDEC_SOUND_FORMAT_UNDEFINED:
    default:
      g_assert_not_reached ();
      return NULL;
  }

  return ret;
}

void
swfdec_sound_add (gint16 *dest, const gint16 *src, unsigned int n_samples, const guint16 volume[2])
{
  unsigned int i;
  int tmp;

  for (i = 0; i < n_samples; i++) {
    tmp = (volume[0] * *src++) >> 16;
    tmp += *dest;
    tmp = CLAMP (tmp, G_MININT16, G_MAXINT16);
    *dest++ = tmp;
    tmp = (volume[1] * *src++) >> 16;
    tmp += *dest;
    tmp = CLAMP (tmp, G_MININT16, G_MAXINT16);
    *dest++ = tmp;
  }
}

/**
 * swfdec_sound_render:
 * @sound: a #SwfdecSound
 * @dest: target to add to
 * @offset: offset in samples into the data
 * @n_samples: amount of samples to render
 * @volume: volume for left and right channel
 *
 * Renders the given sound onto the existing data in @dest.
 **/
void
swfdec_sound_render (SwfdecSound *sound, gint16 *dest, 
    unsigned int offset, unsigned int n_samples, const guint16 volume[2])
{
  gint16 *src;

  g_return_if_fail (SWFDEC_IS_SOUND (sound));
  g_return_if_fail (sound->decoded != NULL);
  g_return_if_fail (offset + n_samples <= sound->n_samples);

  if (sound->decoded == NULL) {
    SWFDEC_ERROR ("can't do on-the-fly decoding yet :(");
    return;
  }
  src = (gint16*) (sound->decoded->data + offset * 4);
  swfdec_sound_add (dest, src, n_samples, volume);
}
