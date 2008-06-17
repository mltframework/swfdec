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
    g_object_unref (stream->decoder);
    stream->decoder = NULL;
  }
  g_queue_foreach (stream->queue, (GFunc) swfdec_buffer_unref, NULL);
  g_queue_free (stream->queue);

  G_OBJECT_CLASS (swfdec_audio_stream_parent_class)->dispose (object);
}

/* returns: number of samples available */
static void
swfdec_audio_stream_require (SwfdecAudioStream *stream, guint n_samples)
{
  SwfdecAudioStreamClass *klass = SWFDEC_AUDIO_STREAM_GET_CLASS (stream);
  SwfdecBuffer *buffer;

  /* subclasses are responsible for having set a proper decoder */
  g_assert (SWFDEC_IS_AUDIO_DECODER (stream->decoder));

  while (stream->queue_size < n_samples && !stream->done) {
    /* if the decoder still has data */
    buffer = swfdec_audio_decoder_pull (stream->decoder);
    if (buffer) {
      g_assert (buffer->length %4 == 0);
      g_queue_push_tail (stream->queue, buffer);
      stream->queue_size += buffer->length / 4;
      continue;
    }
    /* otherwise get a new buffer from the decoder */
    buffer = klass->pull (stream);
    if (buffer == NULL) {
      stream->buffering = TRUE;
      break;
    }
    swfdec_audio_decoder_push (stream->decoder, buffer);
    swfdec_buffer_unref (buffer);
  }
}

static gsize
swfdec_audio_stream_render (SwfdecAudio *audio, gint16* dest,
    gsize start, gsize n_samples)
{
  SwfdecAudioStream *stream = SWFDEC_AUDIO_STREAM (audio);
  GList *walk;
  gsize samples, rendered, skip;
  SwfdecBuffer *buffer;

  g_assert (start < G_MAXINT);
  SWFDEC_LOG ("stream %p rendering offset %"G_GSIZE_FORMAT", samples %"G_GSIZE_FORMAT,
      stream, start, n_samples);
  swfdec_audio_stream_require (stream, start + n_samples);
  if (stream->queue_size <= start)
    return 0;
  n_samples = MIN (stream->queue_size, n_samples + start);

  rendered = 0;
  for (walk = g_queue_peek_head_link (stream->queue); 
       rendered < n_samples; walk = walk->next) {
    /* must hold, we check above that enough data is available */
    g_assert (walk);
    buffer = walk->data;
    samples = buffer->length / 4;
    if (rendered < start) {
      skip = MIN (samples, start - rendered);
      samples -= skip;
      samples = MIN (n_samples - start, samples);
    } else {
      skip = 0;
      samples = MIN (n_samples - rendered, samples);
    }
    samples = MIN (n_samples - MAX (start, rendered), samples);
    if (samples)
      swfdec_sound_buffer_render (dest, buffer, skip, samples);

    rendered += skip + samples;
    dest += 2 * samples;
  }

  return rendered - start;
}

static gboolean
swfdec_audio_stream_check_buffering (SwfdecAudioStream *stream)
{
  SwfdecAudioStreamClass *klass;
  SwfdecBuffer *buffer;

  if (!stream->buffering || stream->done)
    return FALSE;

  klass = SWFDEC_AUDIO_STREAM_GET_CLASS (stream);
  buffer = klass->pull (stream);
  if (buffer == NULL)
    return FALSE;

  swfdec_audio_decoder_push (stream->decoder, buffer);
  swfdec_buffer_unref (buffer);
  stream->buffering = FALSE;
  g_signal_emit_by_name (stream, "new-data");
  return stream->queue_size == 0;
}

static gsize
swfdec_audio_stream_iterate (SwfdecAudio *audio, gsize remove)
{
  SwfdecAudioStream *stream = SWFDEC_AUDIO_STREAM (audio);
  SwfdecBuffer *buffer;
  gsize samples, cur_samples;

  if (swfdec_audio_stream_check_buffering (stream))
    return G_MAXUINT;
  swfdec_audio_stream_require (stream, remove);
  samples = MIN (remove, stream->queue_size);

  while (samples > 0) {
    buffer = g_queue_pop_head (stream->queue);
    cur_samples = buffer->length / 4;
    if (samples < cur_samples) {
      SwfdecBuffer *sub = swfdec_buffer_new_subbuffer (buffer,
	  samples * 4, buffer->length - samples * 4);
      g_queue_push_head (stream->queue, sub);
      cur_samples = samples;
    }
    stream->queue_size -= cur_samples;
    samples -= cur_samples;
  }
  
  if (!stream->done) {
    return G_MAXUINT;
  } else {
    return stream->queue_size;
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
  stream->queue = g_queue_new ();
}

void
swfdec_audio_stream_use_decoder (SwfdecAudioStream *stream,
    guint codec, SwfdecAudioFormat format)
{
  g_return_if_fail (SWFDEC_IS_AUDIO_STREAM (stream));
  g_return_if_fail (SWFDEC_IS_AUDIO_FORMAT (format));

  if (stream->decoder) {
    if (swfdec_audio_decoder_uses_format (stream->decoder, codec, format))
      return;
    /* FIXME: send NULL buffer */
    g_object_unref (stream->decoder);
  }
  stream->decoder = swfdec_audio_decoder_new (codec, format);
}

void
swfdec_audio_stream_done (SwfdecAudioStream *stream)
{
  g_return_if_fail (SWFDEC_IS_AUDIO_STREAM (stream));
  g_return_if_fail (!stream->done);

  stream->done = TRUE;
}

