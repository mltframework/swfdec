/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_audio_flv.h"
#include "swfdec_debug.h"
#include "swfdec_sound.h"


G_DEFINE_TYPE (SwfdecAudioFlv, swfdec_audio_flv, SWFDEC_TYPE_AUDIO)

static void
swfdec_audio_flv_dispose (GObject *object)
{
  SwfdecAudioFlv *flv = SWFDEC_AUDIO_FLV (object);

  if (flv->decoder != NULL) {
    SwfdecBuffer *buffer = swfdec_audio_codec_finish (flv->codec, flv->decoder);
    flv->decoder = NULL;
    if (buffer)
      swfdec_buffer_unref (buffer);
  }
  g_queue_foreach (flv->playback_queue, (GFunc) swfdec_buffer_unref, NULL);
  g_queue_free (flv->playback_queue);
  g_object_unref (flv->flvdecoder);

  G_OBJECT_CLASS (swfdec_audio_flv_parent_class)->dispose (object);
}

static SwfdecBuffer *
swfdec_audio_flv_decode_one (SwfdecAudioFlv *flv)
{
  SwfdecBuffer *buffer;
  SwfdecAudioFormat format;
  gboolean width;
  SwfdecAudioOut in;
  guint now, soon;

  if  (g_queue_is_empty (flv->playback_queue)) {
    /* sync */
    guint last;
    swfdec_flv_decoder_get_audio (flv->flvdecoder, 
	SWFDEC_TICKS_TO_MSECS (flv->timestamp),
	NULL, NULL, NULL, &last, NULL);
    flv->playback_skip = SWFDEC_TICKS_TO_SAMPLES (
	flv->timestamp - SWFDEC_MSECS_TO_TICKS (last));
    flv->next_timestamp = last;
    SWFDEC_DEBUG ("syncing to %ums: next timestamp to decode is %ums, skipping %u samples", 
	(guint) SWFDEC_TICKS_TO_MSECS (flv->timestamp), 
	flv->next_timestamp, flv->playback_skip);
  }
next:
  if (flv->out && flv->next_timestamp == 0) {
    if (flv->decoder == NULL)
      return NULL;
    buffer = swfdec_audio_codec_finish (flv->codec, flv->decoder);
    flv->decoder = NULL;
    if (buffer == NULL)
      return NULL;
  } else {
    buffer = swfdec_flv_decoder_get_audio (flv->flvdecoder, flv->next_timestamp,
      &format, &width, &in, &now, &soon);

    if (flv->next_timestamp != now) {
      /* FIXME: do sync on first frame here */
      SWFDEC_WARNING ("FIXME: didn't get requested timestamp - still loading?");
    }
    /* FIXME FIXME FIXME: This avoids decoding the last frame forever, however it ensures sync */
    if (soon == 0)
      return NULL;
    flv->next_timestamp = soon;
    if (flv->in == 0) {
      /* init */
      if (flv->decoder) {
	swfdec_audio_codec_finish (flv->codec, flv->decoder);
	flv->decoder = NULL;
      }
      flv->format = format;
      flv->width = width;
      flv->in = in;
      flv->codec = swfdec_codec_get_audio (flv->format);
      if (flv->codec) {
	flv->decoder = swfdec_audio_codec_init (flv->codec, flv->format, flv->width, flv->in);
	flv->out = swfdec_audio_codec_get_format (flv->codec, flv->decoder);
      }
    } else if (format != flv->format ||
	width != flv->width ||
	in != flv->in) {
      SWFDEC_ERROR ("FIXME: format change not implemented");
      return NULL;
    }
    if (flv->decoder == NULL)
      return NULL;
    buffer = swfdec_audio_codec_decode (flv->codec, flv->decoder, buffer);
    if (buffer == NULL)
      goto next;
  }

  g_assert (buffer);
  g_queue_push_tail (flv->playback_queue, buffer);
  return buffer;
}

static void
swfdec_audio_flv_render (SwfdecAudio *audio, gint16* dest,
    guint start, guint n_samples)
{
  SwfdecAudioFlv *flv = SWFDEC_AUDIO_FLV (audio);
  GList *walk;
  guint samples;
  SwfdecBuffer *buffer, *previous;

  g_assert (start < G_MAXINT);
  start += flv->playback_skip;
  SWFDEC_LOG ("flv %p rendering offset %u, samples %u", flv, start, n_samples);
  walk = g_queue_peek_head_link (flv->playback_queue);
  previous = NULL;
  while (n_samples) {
    if (walk) {
      buffer = walk->data;
      walk = walk->next;
    } else {
      buffer = swfdec_audio_flv_decode_one (flv);
      if (!buffer)
	break;
    }
    samples = swfdec_sound_buffer_get_n_samples (buffer, flv->out);
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
    samples = MIN (samples, n_samples);
    swfdec_sound_buffer_render (dest, buffer, flv->out, previous, start, samples);
    start = 0;
    n_samples -= samples;
    dest += 2 * samples;
    previous = buffer;
  }
}

static guint
swfdec_audio_flv_iterate (SwfdecAudio *audio, guint remove)
{
  SwfdecAudioFlv *flv = SWFDEC_AUDIO_FLV (audio);
  SwfdecBuffer *buffer;
  guint next;

  flv->playback_skip += remove;
  buffer = g_queue_peek_head (flv->playback_queue);
  while (buffer && flv->playback_skip >= 
	 swfdec_sound_buffer_get_n_samples (buffer, flv->out) 
	 + SWFDEC_AUDIO_OUT_GRANULARITY (flv->out)) {
    buffer = g_queue_pop_head (flv->playback_queue);
    SWFDEC_LOG ("removing buffer with %u samples", 
	swfdec_sound_buffer_get_n_samples (buffer, flv->out));
    flv->playback_skip -= swfdec_sound_buffer_get_n_samples (buffer, flv->out);
    swfdec_buffer_unref (buffer);
    buffer = g_queue_peek_head (flv->playback_queue);
  }
  flv->timestamp += SWFDEC_SAMPLES_TO_TICKS (remove);
  
  if (!g_queue_is_empty (flv->playback_queue))
    return G_MAXUINT;
  swfdec_flv_decoder_get_audio (flv->flvdecoder,
      SWFDEC_TICKS_TO_MSECS (flv->timestamp),
      NULL, NULL, NULL, NULL, &next);
  return next ? G_MAXUINT : 0;
}

static void
swfdec_audio_flv_class_init (SwfdecAudioFlvClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioClass *audio_class = SWFDEC_AUDIO_CLASS (klass);

  object_class->dispose = swfdec_audio_flv_dispose;

  audio_class->iterate = swfdec_audio_flv_iterate;
  audio_class->render = swfdec_audio_flv_render;
}

static void
swfdec_audio_flv_init (SwfdecAudioFlv *flv)
{
  flv->playback_queue = g_queue_new ();
}

SwfdecAudio *
swfdec_audio_flv_new (SwfdecPlayer *player, SwfdecFlvDecoder *decoder, guint timestamp)
{
  SwfdecAudioFlv *flv;
  
  flv = g_object_new (SWFDEC_TYPE_AUDIO_FLV, NULL);

  SWFDEC_DEBUG ("new audio flv for decoder %p, starting at %ums", 
      decoder, timestamp);
  g_object_ref (decoder);
  flv->flvdecoder = decoder;
  flv->timestamp = SWFDEC_MSECS_TO_TICKS (timestamp);
  swfdec_audio_add (SWFDEC_AUDIO (flv), player);

  return SWFDEC_AUDIO (flv);
}

