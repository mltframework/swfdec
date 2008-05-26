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
#include "swfdec_audio_stream.h"
#include "swfdec_debug.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"


G_DEFINE_TYPE (SwfdecAudioStream, swfdec_audio_stream, SWFDEC_TYPE_AUDIO)

static void
swfdec_audio_stream_dispose (GObject *object)
{
  SwfdecAudioStream *stream = SWFDEC_AUDIO_STREAM (object);

  if (stream->decoder != NULL) {
    swfdec_audio_decoder_free (stream->decoder);
    stream->decoder = NULL;
  }
  g_queue_foreach (stream->playback_queue, (GFunc) swfdec_buffer_unref, NULL);
  g_queue_free (stream->playback_queue);

  G_OBJECT_CLASS (swfdec_audio_stream_parent_class)->dispose (object);
}

static SwfdecBuffer *
swfdec_audio_stream_decode_one (SwfdecAudioStream *stream)
{
  SwfdecSpriteFrame *frame;
  SwfdecBuffer *buffer;

  g_assert (!stream->done);
  if (stream->decoder == NULL)
    return NULL;

  while (!(buffer = swfdec_audio_decoder_pull (stream->decoder)) &&
         !stream->done) {
    if (stream->current_frame >= stream->sprite->n_frames)
      goto end;
    frame = &stream->sprite->frames[stream->current_frame];
    stream->current_frame++;
    if (frame->sound_head != stream->sound) 
      goto end;
    if (frame->sound_samples == 0)
      continue;
    if (frame->sound_block)
      swfdec_audio_decoder_push (stream->decoder, frame->sound_block);
    continue;
end:
    swfdec_audio_decoder_push (stream->decoder, NULL);
    stream->done = TRUE;
  }
  return buffer;
}

static guint
swfdec_audio_stream_render (SwfdecAudio *audio, gint16* dest,
    guint start, guint n_samples)
{
  SwfdecAudioStream *stream = SWFDEC_AUDIO_STREAM (audio);
  GList *walk;
  guint samples, rendered;
  SwfdecBuffer *buffer;

  g_assert (start < G_MAXINT);
  start += stream->playback_skip;
  SWFDEC_LOG ("stream %p rendering offset %u, samples %u", stream, start, n_samples);
  walk = g_queue_peek_head_link (stream->playback_queue);
  for (rendered = 0; rendered < n_samples;) {
    if (walk) {
      buffer = walk->data;
      walk = walk->next;
    } else {
      if (stream->done)
	break;
      buffer = swfdec_audio_stream_decode_one (stream);
      if (!buffer)
	break;
      g_queue_push_tail (stream->playback_queue, buffer);
    }
    samples = swfdec_sound_buffer_get_n_samples (buffer, 
	swfdec_audio_format_new (44100, 2, TRUE));
    if (start) {
      if (samples <= start) {
	start -= samples;
	continue;
      }
      samples -= start;
      SWFDEC_LOG ("rendering %u samples, skipping %u",
	  samples, start);
    } else {
      SWFDEC_LOG ("rendering %u samples", samples);
    }
    samples = MIN (samples, n_samples - rendered);
    swfdec_sound_buffer_render (dest, buffer, start, samples);
    start = 0;
    rendered += samples;
    dest += 2 * samples;
  }

  return rendered;
}

static guint
swfdec_audio_stream_iterate (SwfdecAudio *audio, guint remove)
{
  SwfdecAudioStream *stream = SWFDEC_AUDIO_STREAM (audio);
  SwfdecBuffer *buffer;

  stream->playback_skip += remove;
  buffer = g_queue_peek_head (stream->playback_queue);
  while (buffer && stream->playback_skip >= 
	 swfdec_sound_buffer_get_n_samples (buffer, swfdec_audio_format_new (44100, 2, TRUE))
	 + swfdec_audio_format_get_granularity (swfdec_audio_format_new (44100, 2, TRUE))) {
    buffer = g_queue_pop_head (stream->playback_queue);
    SWFDEC_LOG ("removing buffer with %u samples", 
	swfdec_sound_buffer_get_n_samples (buffer, 
	  swfdec_audio_format_new (44100, 2, TRUE)));
    stream->playback_skip -= swfdec_sound_buffer_get_n_samples (buffer, 
	swfdec_audio_format_new (44100, 2, TRUE));
    swfdec_buffer_unref (buffer);
    buffer = g_queue_peek_head (stream->playback_queue);
  }
  
  if (!stream->done) {
    return G_MAXUINT;
  } else {
    GList *walk;
    guint ret = 0;
    SwfdecAudioFormat format = swfdec_audio_format_new (44100, 2, TRUE);
    
    for (walk = g_queue_peek_head_link (stream->playback_queue); walk; walk = walk->next) {
      ret += swfdec_sound_buffer_get_n_samples (walk->data, format);
    }
    return ret - stream->playback_skip;
  }
}

static void
swfdec_audio_stream_class_init (SwfdecAudioStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioClass *audio_class = SWFDEC_AUDIO_CLASS (klass);

  object_class->dispose = swfdec_audio_stream_dispose;

  audio_class->iterate = swfdec_audio_stream_iterate;
  audio_class->render = swfdec_audio_stream_render;
}

static void
swfdec_audio_stream_init (SwfdecAudioStream *stream)
{
  stream->playback_queue = g_queue_new ();
}

SwfdecAudio *
swfdec_audio_stream_new (SwfdecPlayer *player, SwfdecSprite *sprite, guint start_frame)
{
  SwfdecAudioStream *stream;
  SwfdecSpriteFrame *frame;
  
  stream = g_object_new (SWFDEC_TYPE_AUDIO_STREAM, NULL);

  SWFDEC_DEBUG ("new audio stream for sprite %d, starting at %u", 
      SWFDEC_CHARACTER (sprite)->id, start_frame);
  stream->sprite = sprite;
  frame = &sprite->frames[start_frame];
  g_assert (frame->sound_head);
  stream->sound = frame->sound_head;
  stream->playback_skip = frame->sound_skip;
  stream->current_frame = start_frame;
  stream->decoder = swfdec_audio_decoder_new (stream->sound->codec, 
      stream->sound->format);
  swfdec_audio_add (SWFDEC_AUDIO (stream), player);

  return SWFDEC_AUDIO (stream);
}

