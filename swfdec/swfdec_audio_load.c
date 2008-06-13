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
#include "swfdec_audio_load.h"
#include "swfdec_debug.h"
#include "swfdec_sprite.h"
#include "swfdec_tag.h"


G_DEFINE_TYPE (SwfdecAudioLoad, swfdec_audio_load, SWFDEC_TYPE_AUDIO_STREAM)

static void
swfdec_audio_load_dispose (GObject *object)
{
  SwfdecAudioLoad *stream = SWFDEC_AUDIO_LOAD (object);

  if (stream->load != NULL) {
    g_object_unref (stream->load);
    stream->load = NULL;
  }

  G_OBJECT_CLASS (swfdec_audio_load_parent_class)->dispose (object);
}

static SwfdecBuffer *
swfdec_audio_load_pull (SwfdecAudioStream *audio)
{
  SwfdecAudioLoad *stream = SWFDEC_AUDIO_LOAD (audio);

  if (stream->frame >= stream->load->frames->len) {
    if (stream->load->stream == NULL)
      swfdec_audio_stream_done (audio);
    return NULL;
  }

  return swfdec_buffer_ref (g_ptr_array_index (stream->load->frames, stream->frame++));
}

static void
swfdec_audio_load_class_init (SwfdecAudioLoadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioStreamClass *stream_class = SWFDEC_AUDIO_STREAM_CLASS (klass);

  object_class->dispose = swfdec_audio_load_dispose;

  stream_class->pull = swfdec_audio_load_pull;
}

static void
swfdec_audio_load_init (SwfdecAudioLoad *stream)
{
}

SwfdecAudio *
swfdec_audio_load_new (SwfdecPlayer *player, SwfdecLoadSound *load)
{
  SwfdecAudioLoad *stream;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_LOAD_SOUND (load), NULL);

  stream = g_object_new (SWFDEC_TYPE_AUDIO_LOAD, NULL);
  stream->load = g_object_ref (load);
  swfdec_audio_stream_use_decoder (SWFDEC_AUDIO_STREAM (stream), 
      SWFDEC_AUDIO_CODEC_MP3, swfdec_audio_format_new (44100, TRUE, 2));
  
  swfdec_audio_add (SWFDEC_AUDIO (stream), player);

  return SWFDEC_AUDIO (stream);
}

