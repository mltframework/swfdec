/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_audio_swf_stream.h"
#include "swfdec_debug.h"
#include "swfdec_sprite.h"
#include "swfdec_tag.h"


G_DEFINE_TYPE (SwfdecAudioSwfStream, swfdec_audio_swf_stream, SWFDEC_TYPE_AUDIO_STREAM)

static void
swfdec_audio_swf_stream_dispose (GObject *object)
{
  SwfdecAudioSwfStream *stream = SWFDEC_AUDIO_SWF_STREAM (object);

  if (stream->sprite != NULL) {
    g_object_unref (stream->sprite);
    stream->sprite = NULL;
  }

  G_OBJECT_CLASS (swfdec_audio_swf_stream_parent_class)->dispose (object);
}

static void
swfdec_audio_swf_stream_head (SwfdecAudioSwfStream *stream, SwfdecBuffer *buffer)
{
  SwfdecBits bits;
  SwfdecAudioFormat playback_format, format;
  guint playback_codec, codec;
  int n_samples;
  guint latency = 0;

  swfdec_bits_init (&bits, buffer);

  /* we don't care about playback suggestions */
  playback_codec = swfdec_bits_getbits (&bits, 4);
  playback_format = swfdec_audio_format_parse (&bits);
  SWFDEC_LOG ("  suggested playback format: %s", swfdec_audio_format_to_string (playback_format));

  codec = swfdec_bits_getbits (&bits, 4);
  format = swfdec_audio_format_parse (&bits);
  n_samples = swfdec_bits_get_u16 (&bits);
  SWFDEC_LOG ("  codec: %u", codec);
  SWFDEC_LOG ("  format: %s", swfdec_audio_format_to_string (format));
  SWFDEC_LOG ("  samples: %u", n_samples);

  switch (codec) {
    case SWFDEC_AUDIO_CODEC_UNDEFINED:
      if (swfdec_audio_format_is_16bit (format)) {
	SWFDEC_WARNING ("undefined endianness for s16 sound");
	/* just assume LE and hope it works (FIXME: want a switch for this?) */
	codec = SWFDEC_AUDIO_CODEC_UNCOMPRESSED;
      }
      break;
    case SWFDEC_AUDIO_CODEC_MP3:
      /* latency seek */
      latency = swfdec_bits_get_u16 (&bits);
      break;
    case SWFDEC_AUDIO_CODEC_ADPCM:
    case SWFDEC_AUDIO_CODEC_UNCOMPRESSED:
    case SWFDEC_AUDIO_CODEC_NELLYMOSER_8KHZ:
    case SWFDEC_AUDIO_CODEC_NELLYMOSER:
      break;
    default:
      SWFDEC_WARNING ("unknown codec %u", codec);
      break;
  }

  swfdec_audio_stream_use_decoder (SWFDEC_AUDIO_STREAM (stream), codec, format);
}

static SwfdecBuffer *
swfdec_audio_swf_stream_block (SwfdecAudioSwfStream *stream, SwfdecBuffer *buffer)
{
  SwfdecBits bits;
  guint n_samples;
  int skip;

  swfdec_bits_init (&bits, buffer);

  /* FIXME: we want accessor functions for this */
  if (SWFDEC_AUDIO_STREAM (stream)->decoder->codec == SWFDEC_AUDIO_CODEC_MP3) {
    n_samples = swfdec_bits_get_u16 (&bits);
    skip = swfdec_bits_get_s16 (&bits);
  }
  buffer = swfdec_bits_get_buffer (&bits, -1);
  /* use this to write out the stream data to stdout - nice way to get an mp3 file :) */
  //write (1, (void *) buffer->data, buffer->length);

  return buffer;
}

static SwfdecBuffer *
swfdec_audio_swf_stream_pull (SwfdecAudioStream *audio)
{
  SwfdecAudioSwfStream *stream = SWFDEC_AUDIO_SWF_STREAM (audio);
  SwfdecBuffer *buffer;
  guint tag;

  do {
    if (!swfdec_sprite_get_action (stream->sprite, stream->id, &tag, &buffer)) {
      if (swfdec_sprite_is_loaded (stream->sprite))
	swfdec_audio_stream_done (audio);
      buffer = NULL;
      break;
    }
    stream->id++;
    switch (tag) {
      case SWFDEC_TAG_SOUNDSTREAMHEAD:
      case SWFDEC_TAG_SOUNDSTREAMHEAD2:
	swfdec_audio_swf_stream_head (stream, buffer);
	break;
      case SWFDEC_TAG_SOUNDSTREAMBLOCK:
	buffer = swfdec_audio_swf_stream_block (stream, buffer);
	break;
      default:
	break;
    }
  } while (tag != SWFDEC_TAG_SOUNDSTREAMBLOCK);

  return buffer;
}

static void
swfdec_audio_swf_stream_class_init (SwfdecAudioSwfStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioStreamClass *stream_class = SWFDEC_AUDIO_STREAM_CLASS (klass);

  object_class->dispose = swfdec_audio_swf_stream_dispose;

  stream_class->pull = swfdec_audio_swf_stream_pull;
}

static void
swfdec_audio_swf_stream_init (SwfdecAudioSwfStream *stream)
{
}

static void
check (SwfdecAudio *audio)
{
  guint length = 20000;
  gint16 *data = g_new (gint16, 2 * length);
  gint16 *compare = g_new (gint16, 2 * length);
  guint i;

  swfdec_audio_render (audio, compare, 0, length);
  for (i = 1; i < length; i++) {
    swfdec_audio_render (audio, data, i, length - i);
    g_assert (memcmp (data, compare + 2 * i, (length - i) * 4) == 0);
  }

  g_free (data);
  g_free (compare);
}

SwfdecAudio *
swfdec_audio_swf_stream_new (SwfdecPlayer *player, SwfdecSprite *sprite,
    guint id)
{
  SwfdecAudioSwfStream *stream;
  guint i, tag;
  SwfdecBuffer *buffer;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_SPRITE (sprite), NULL);

  stream = g_object_new (SWFDEC_TYPE_AUDIO_SWF_STREAM, NULL);
  stream->sprite = g_object_ref (sprite);
  stream->id = id;
  
  i = id;
  do {
    i--;
    if (!swfdec_sprite_get_action (sprite, i, &tag, &buffer)) {
      g_assert_not_reached ();
    }
    if (tag == SWFDEC_TAG_SOUNDSTREAMHEAD ||
	tag == SWFDEC_TAG_SOUNDSTREAMHEAD2) {
      swfdec_audio_swf_stream_head (stream, buffer);
      goto found;
    }
  } while (i > 0);
  SWFDEC_ERROR ("No SoundStreamHead tag found in sprite %u", 
      SWFDEC_CHARACTER (sprite)->id);
  swfdec_audio_stream_done (SWFDEC_AUDIO_STREAM (stream));

found:
  swfdec_audio_add (SWFDEC_AUDIO (stream), player);
  check (SWFDEC_AUDIO (stream));
  return SWFDEC_AUDIO (stream);
}

