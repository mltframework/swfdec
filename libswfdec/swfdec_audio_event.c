/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_audio_event.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"


G_DEFINE_TYPE (SwfdecAudioEvent, swfdec_audio_event, SWFDEC_TYPE_AUDIO)


static guint
swfdec_audio_event_iterate (SwfdecAudio *audio, guint remove)
{
  SwfdecAudioEvent *event = SWFDEC_AUDIO_EVENT (audio);

  if (event->n_samples == 0)
    return 0;

  event->offset += remove;
  event->loop += event->offset / event->n_samples;
  event->offset %= event->n_samples;
  
  if (event->loop < event->n_loops)
    return event->n_samples * (event->n_loops - event->loop) - event->offset;
  else
    return 0;
}

static void
swfdec_audio_event_render (SwfdecAudio *audio, gint16* dest,
    guint start, guint n_samples)
{
  SwfdecAudioEvent *event = SWFDEC_AUDIO_EVENT (audio);
  guint offset = event->offset + start;
  guint loop;
  guint samples;

  loop = event->loop + offset / event->n_samples;
  offset %= event->n_samples;
  for (; loop < event->n_loops && n_samples > 0; loop++) {
    samples = MIN (n_samples, event->n_samples - offset);
    swfdec_sound_buffer_render	(dest, event->decoded, event->decoded_format,
	loop == 0 ? NULL : event->decoded, offset, samples);
    n_samples -= samples;
    dest += samples * 4;
    offset = 0;
  }
}

static void
swfdec_audio_event_dispose (GObject *object)
{
  SwfdecAudioEvent *audio = SWFDEC_AUDIO_EVENT (object);

  g_free (audio->envelope);
  audio->envelope = NULL;
  audio->n_envelopes = 0;

  G_OBJECT_CLASS (swfdec_audio_event_parent_class)->dispose (object);
}

static void
swfdec_audio_event_class_init (SwfdecAudioEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioClass *audio_class = SWFDEC_AUDIO_CLASS (klass);

  object_class->dispose = swfdec_audio_event_dispose;

  audio_class->iterate = swfdec_audio_event_iterate;
  audio_class->render = swfdec_audio_event_render;
}

static void
swfdec_audio_event_init (SwfdecAudioEvent *audio_event)
{
}

static void
swfdec_audio_event_decode (SwfdecAudioEvent *event)
{
  guint granule, bytes_per_sample;

  event->decoded = swfdec_sound_get_decoded (event->sound,
      &event->decoded_format);
  if (event->decoded == NULL) {
    SWFDEC_INFO ("Could not decode audio. Will assume %u samples of silence instead.",
	event->sound->n_samples);
    event->decoded = swfdec_buffer_new_and_alloc0 (event->sound->n_samples / 4);
    event->decoded_format = swfdec_audio_format_new (5512, 1, FALSE);
  } else {
    swfdec_buffer_ref (event->decoded);
  }
  granule = swfdec_audio_format_get_granularity (event->decoded_format);
  bytes_per_sample = swfdec_audio_format_get_channels (event->decoded_format) *
      (swfdec_audio_format_is_16bit (event->decoded_format) ? 2 : 1);
  if (event->start_sample) {
    guint skip;
    if (event->start_sample % granule) {
      SWFDEC_FIXME ("figure out how high resolution start samples work");
    }
    skip = bytes_per_sample * (event->start_sample / granule);
    if (skip >= event->decoded->length) {
      SWFDEC_WARNING ("start sample %u > total number of samples %u",
	  event->start_sample / granule, event->decoded->length / bytes_per_sample);
      swfdec_buffer_unref (event->decoded);
      event->decoded = swfdec_buffer_new ();
    } else {
      SwfdecBuffer *sub = swfdec_buffer_new_subbuffer (event->decoded,
	  skip, event->decoded->length - skip);
      swfdec_buffer_unref (event->decoded);
      event->decoded = sub;
    }
  }
  if (event->stop_sample) {
    guint keep;
    if (event->stop_sample % granule) {
      SWFDEC_FIXME ("figure out how high resolution stop samples work");
    }
    keep = bytes_per_sample * (event->stop_sample / granule - event->start_sample / granule);
    if (keep > event->decoded->length) {
      SWFDEC_WARNING ("stop sample %u outside of decoded number of samples %u",
	  event->stop_sample / granule, event->decoded->length / bytes_per_sample +
	  event->start_sample / granule);
    } else if (keep < event->decoded->length) {
      SwfdecBuffer *sub = swfdec_buffer_new_subbuffer (event->decoded,
	  0, keep);
      swfdec_buffer_unref (event->decoded);
      event->decoded = sub;
    }
  }
  event->n_samples = event->decoded->length / bytes_per_sample * granule;
}

static SwfdecAudioEvent *
swfdec_audio_event_create (SwfdecSound *sound, guint offset, guint end_offset, guint n_loops)
{
  SwfdecAudioEvent *event;
  
  event = g_object_new (SWFDEC_TYPE_AUDIO_EVENT, NULL);
  event->sound = sound;
  event->start_sample = offset;
  event->n_loops = n_loops;
  event->stop_sample = end_offset;
  swfdec_audio_event_decode (event);
  event->offset = 0;

  return event;
}

/**
 * swfdec_audio_event_new:
 * @player: the #SwfdecPlayer to play the sound in
 * @sound: the sound to be played
 * @offset: offset into sound at which to start playing
 * @n_loops: number of times the sound should be played
 *
 * Starts playing back a sound from the given offset and loops the sound 
 * @n_loops times.
 *
 * Returns: a new #SwfdecAudio
 **/
SwfdecAudio *
swfdec_audio_event_new (SwfdecPlayer *player, SwfdecSound *sound, guint	offset,
    guint n_loops)
{
  SwfdecAudioEvent *event;

  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_SOUND (sound), NULL);

  event = swfdec_audio_event_create (sound, offset, 0, n_loops);
  swfdec_audio_add (SWFDEC_AUDIO (event), player);

  return SWFDEC_AUDIO (event);
}

static SwfdecAudio *
swfdec_audio_event_get (SwfdecPlayer *player, SwfdecSound *sound)
{
  GList *walk;

  if (player == NULL)
    return NULL;

  for (walk = player->audio; walk; walk = walk->next) {
    SwfdecAudio *audio = walk->data;
    if (!SWFDEC_IS_AUDIO_EVENT (audio))
      continue;
    if (SWFDEC_AUDIO_EVENT (audio)->sound == sound) {
      return audio;
    }
  }
  return NULL;
}

/**
 * swfdec_audio_event_new_from_chunk:
 * @player: a #SwfdecPlayer or NULL
 * @event: a sound event to start playing back
 *
 * Starts playback of the given sound event (or, when @player is NULL, creates
 * an element for playing back the given sound).
 *
 * Returns: the sound effect or NULL if no new sound was created.
 **/
SwfdecAudio *
swfdec_audio_event_new_from_chunk (SwfdecPlayer *player, SwfdecSoundChunk *chunk)
{
  SwfdecAudioEvent *event;

  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (chunk != NULL, NULL);

  if (chunk->stop) {
    SwfdecAudio *audio = swfdec_audio_event_get (player, chunk->sound);
    if (audio) {
      SWFDEC_LOG ("stopping sound %d", SWFDEC_CHARACTER (chunk->sound)->id);
      swfdec_audio_remove (audio);
    }
    return NULL;
  }
  SWFDEC_LOG ("adding sound %d to playing sounds", SWFDEC_CHARACTER (chunk->sound)->id);
  if (chunk->no_restart &&
      (event = (SwfdecAudioEvent *) swfdec_audio_event_get (player, chunk->sound))) {
    SWFDEC_DEBUG ("sound %d is already playing, reusing it", 
	SWFDEC_CHARACTER (event->sound)->id);
    g_object_ref (event);
    return SWFDEC_AUDIO (event);
  }
  event = swfdec_audio_event_create (chunk->sound, chunk->start_sample, 
      chunk->stop_sample, chunk->loop_count);
  event->n_envelopes = chunk->n_envelopes;
  if (event->n_envelopes)
    event->envelope = g_memdup (chunk->envelope, sizeof (SwfdecSoundEnvelope) * event->n_envelopes);
  SWFDEC_DEBUG ("playing sound %d from offset %d now", SWFDEC_CHARACTER (event->sound)->id,
      event->start_sample);
  swfdec_audio_add (SWFDEC_AUDIO (event), player);

  return SWFDEC_AUDIO (event);
}

