/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "swfdec.h"
#include "swfdec_sound.h"
#include "swfdec_bits.h"
#include "swfdec_buffer.h"
#include "swfdec_button.h"
#include "swfdec_debug.h"
#include "swfdec_sprite.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecSound, swfdec_sound, SWFDEC_TYPE_CHARACTER)

static void
swfdec_sound_dispose (GObject *object)
{
  SwfdecSound * sound = SWFDEC_SOUND (object);

  if (sound->decoded)
    swfdec_buffer_unref (sound->decoded);

  G_OBJECT_CLASS (swfdec_sound_parent_class)->dispose (object);
}

static void
swfdec_sound_class_init (SwfdecSoundClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_sound_dispose;
}

static void
swfdec_sound_init (SwfdecSound * sound)
{

}

int
tag_func_sound_stream_block (SwfdecSwfDecoder * s)
{
  SwfdecSound *sound;
  SwfdecBuffer *chunk;
  int n_samples;
  int skip;

  /* for MPEG, data starts after 4 byte header */

  sound = SWFDEC_SOUND (s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head);

  if (!sound) {
    SWFDEC_WARNING ("no streaming sound block");
    return SWFDEC_STATUS_OK;
  }

  n_samples = swfdec_bits_get_u16 (&s->b);
  if (sound->format == SWFDEC_AUDIO_FORMAT_MP3) {
    skip = swfdec_bits_get_s16 (&s->b);
  } else {
    skip = 0;
  }
  if (s->b.ptr == s->b.end) {
    SWFDEC_DEBUG ("empty sound block n_samples=%d skip=%d", n_samples,
        skip);
    return SWFDEC_STATUS_OK;
  }

  chunk = swfdec_bits_get_buffer (&s->b, -1);
  if (chunk == NULL) {
    SWFDEC_ERROR ("empty sound chunk");
    return SWFDEC_STATUS_OK;
  }
  SWFDEC_LOG ("got a buffer with %u samples, %d skip and %u bytes mp3 data", n_samples, skip,
      chunk->length);
  /* use this to write out the stream data to stdout - nice way to get an mp3 file :) */
  //write (1, (void *) chunk->data, chunk->length);

  swfdec_sprite_add_sound_chunk (s->parse_sprite, s->parse_sprite->parse_frame, chunk, skip, n_samples);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_sound (SwfdecSwfDecoder * s)
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

  sound = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SOUND);
  if (!sound)
    return SWFDEC_STATUS_OK;

  sound->width = size;
  sound->channels = type ? 2 : 1;
  sound->rate_multiplier = (1 << (3 - rate));
  sound->n_samples = n_samples * sound->rate_multiplier;
  SWFDEC_DEBUG ("%sLE, %uch, %ukHz", size ? "S16" : "U8", sound->channels,
      44100 / sound->rate_multiplier);

  switch (format) {
    case 0:
      if (size == 1)
	SWFDEC_WARNING ("undefined endianness for s16 sound");
      /* just assume LE and hope it works (FIXME: want a switch for this?) */
      /* fall through */
    case 3:
      sound->format = SWFDEC_AUDIO_FORMAT_UNCOMPRESSED;
      orig_buffer = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 2:
      sound->format = SWFDEC_AUDIO_FORMAT_MP3;
      /* FIXME: skip these samples */
      skip = swfdec_bits_get_u16 (b);
      skip *= sound->rate_multiplier;
      orig_buffer = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 1:
      sound->format = SWFDEC_AUDIO_FORMAT_ADPCM;
      orig_buffer = swfdec_bits_get_buffer (&s->b, -1);
      break;
    case 6:
      sound->format = SWFDEC_AUDIO_FORMAT_NELLYMOSER;
      SWFDEC_WARNING ("Nellymoser compression not implemented");
      break;
    default:
      SWFDEC_WARNING ("unknown format %d", format);
      sound->format = format;
  }
  sound->codec = swfdec_codec_get_audio (sound->format);
  if (sound->codec == NULL) {
    return SWFDEC_STATUS_OK;
  }
  if (orig_buffer) {
    SwfdecBuffer *tmp;
    gpointer data = swfdec_sound_init_decoder (sound);
    tmp = swfdec_sound_decode_buffer (sound, data, orig_buffer);
    swfdec_sound_finish_decoder (sound, data);
    swfdec_buffer_unref (orig_buffer);
    if (tmp) {
      SWFDEC_LOG ("after decoding, got %u samples, should get %u and skip %u", tmp->length / 4, sound->n_samples, skip);
      if (skip) {
	SwfdecBuffer *tmp2 = swfdec_buffer_new_subbuffer (tmp, skip, tmp->length - skip);
	swfdec_buffer_unref (tmp);
	tmp = tmp2;
      }
      /* sound buffer may be bigger due to mp3 not having sample boundaries */
      if (tmp->length > sound->n_samples * 4) {
	SwfdecBuffer *tmp2 = swfdec_buffer_new_subbuffer (tmp, 0, sound->n_samples * 4);
	swfdec_buffer_unref (tmp);
	tmp = tmp2;
      }
      /* only assign here, the decoding code checks this variable */
      sound->decoded = tmp;
      if (tmp->length < sound->n_samples * 4) {
	SWFDEC_ERROR ("%u samples in %u bytes should be available, but only %u bytes are",
	    sound->n_samples, sound->n_samples * 4, tmp->length);
	g_assert_not_reached ();
      }
    } else {
      SWFDEC_ERROR ("failed decoding given data in format %u", format);
    }
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
    SWFDEC_ERROR ("defective sound object (id %d)", SWFDEC_CHARACTER (sound)->id);
    s->characters = g_list_remove (s->characters, sound);
    g_object_unref (sound);
  }

  return SWFDEC_STATUS_OK;
}

int
tag_func_sound_stream_head (SwfdecSwfDecoder * s)
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

  sound = g_object_new (SWFDEC_TYPE_SOUND, NULL);

  if (s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head)
    g_object_unref (s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head);
  s->parse_sprite->frames[s->parse_sprite->parse_frame].sound_head = sound;

  sound->width = size;
  sound->channels = type ? 2 : 1;
  sound->rate_multiplier = (1 << (3 - rate));

  switch (format) {
    case 0:
      if (size == 1)
	SWFDEC_WARNING ("undefined endianness for s16 sound");
      /* just assume LE and hope it works (FIXME: want a switch for this?) */
      /* fall through */
    case 3:
      sound->format = SWFDEC_AUDIO_FORMAT_UNCOMPRESSED;
      break;
    case 2:
      sound->format = SWFDEC_AUDIO_FORMAT_MP3;
      /* latency seek */
      latency = swfdec_bits_get_s16 (b);
      break;
    case 1:
      sound->format = SWFDEC_AUDIO_FORMAT_ADPCM;
      SWFDEC_WARNING ("adpcm decoding doesn't work");
      break;
    case 6:
      sound->format = SWFDEC_AUDIO_FORMAT_NELLYMOSER;
      SWFDEC_WARNING ("Nellymosere compression not implemented");
      break;
    default:
      SWFDEC_WARNING ("unknown format %d", format);
      sound->format = format;
  }
  sound->codec = swfdec_codec_get_audio (sound->format);

  return SWFDEC_STATUS_OK;
}

void
swfdec_sound_chunk_free (SwfdecSoundChunk *chunk)
{
  g_return_if_fail (chunk != NULL);

  g_free (chunk->envelope);
  g_free (chunk);
}

SwfdecSoundChunk *
swfdec_sound_parse_chunk (SwfdecSwfDecoder *s, int id)
{
  int has_envelope;
  int has_loops;
  int has_out_point;
  int has_in_point;
  unsigned int i;
  SwfdecSound *sound;
  SwfdecSoundChunk *chunk;
  SwfdecBits *b = &s->b;

  sound = swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_SOUND (sound)) {
    SWFDEC_ERROR ("given id %d does not reference a sound object", id);
    return NULL;
  }

  chunk = g_new0 (SwfdecSoundChunk, 1);
  chunk->sound = sound;
  SWFDEC_LOG ("parsing sound chunk for sound %d", SWFDEC_CHARACTER (sound)->id);

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
tag_func_start_sound (SwfdecSwfDecoder * s)
{
  SwfdecBits *b = &s->b;
  SwfdecSoundChunk *chunk;
  int id;
  SwfdecSpriteFrame *frame = &s->parse_sprite->frames[s->parse_sprite->parse_frame];

  id = swfdec_bits_get_u16 (b);

  chunk = swfdec_sound_parse_chunk (s, id);
  if (chunk) {
    /* append to keep order */
    SWFDEC_DEBUG ("appending StartSound event for sound %u to frame %u\n", id,
	s->parse_sprite->parse_frame);
    frame->sound = g_slist_append (frame->sound, chunk);
  }

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_button_sound (SwfdecSwfDecoder * s)
{
  unsigned int i;
  unsigned int id;
  SwfdecButton *button;

  id = swfdec_bits_get_u16 (&s->b);
  button = (SwfdecButton *) swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_BUTTON (button)) {
    SWFDEC_ERROR ("id %u is not a button", id);
    return SWFDEC_STATUS_OK;
  }
  SWFDEC_LOG ("loading sound events for button %u", id);
  for (i = 0; i < 4; i++) {
    id = swfdec_bits_get_u16 (&s->b);
    if (id) {
      SWFDEC_LOG ("loading sound %u for button event %u", id, i);
      if (button->sounds[i]) {
	SWFDEC_ERROR ("need to delete previous sound for button %u's event %u", 
	    SWFDEC_CHARACTER (button)->id, i);
	swfdec_sound_chunk_free (button->sounds[i]);
      }
      button->sounds[i] = swfdec_sound_parse_chunk (s, id);
    }
  }

  return SWFDEC_STATUS_OK;
}

gpointer 
swfdec_sound_init_decoder (SwfdecSound * sound)
{
  g_assert (sound->decoded == NULL);

  if (sound->codec)
    return swfdec_codec_init (sound->codec, sound->width, sound->channels, sound->rate_multiplier);
  else
    return NULL;
}

void
swfdec_sound_finish_decoder (SwfdecSound * sound, gpointer data)
{
  g_assert (sound->decoded == NULL);

  if (sound->codec)
    swfdec_codec_finish (sound->codec, data);
}

SwfdecBuffer *
swfdec_sound_decode_buffer (SwfdecSound *sound, gpointer data, SwfdecBuffer *buffer)
{
  g_assert (sound->decoded == NULL);

  if (sound->codec)
    return swfdec_codec_decode (sound->codec, data, buffer);
  else
    return NULL;
}

static void
swfdec_sound_add_with_volume (gint16 *dest, const gint16 *src, unsigned int n_samples, const guint16 volume[2])
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

void
swfdec_sound_add (gint16 *dest, const gint16 *src, unsigned int n_samples)
{
  unsigned int i;
  int tmp;

  for (i = 0; i < n_samples; i++) {
    tmp = *src++ + *dest;
    tmp = CLAMP (tmp, G_MININT16, G_MAXINT16);
    *dest++ = tmp;
    tmp = *src++ + *dest;
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
 * @volume: volume for left and right channel or NULL for no volume change
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
  if (volume)
    swfdec_sound_add_with_volume (dest, src, n_samples, volume);
  else
    swfdec_sound_add (dest, src, n_samples);
}
