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
#include "swfdec_audio_internal.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

/**
 * SECTION:SwfdecAudio
 * @title: SwfdecAudio
 * @see_also: SwfdecPlayer
 * @short_description: object used for audio output
 *
 * SwfdecAudio is the way audio output is provided by a #SwfdecPlayer. See
 * its documentation on how to access #SwfdecAudio objects.
 *
 * An audio object gives access to one audio stream played inside a player.
 * You are responsible for outputting this data, swfdec does not try to do this
 * for you.
 *
 * Audio data is always provided in 16bit host-endian stereo. If the data was
 * encoded into a different format originally, Swfdec will already have decoded 
 * it. The data is always referenced relative to the player. Sample 0 
 * references the first sample to be played at the current position. If the 
 * player gets iterated, sample 0 changes. There is no way to access samples
 * belonging to a previous state.
 */

/**
 * SwfdecAudio
 *
 * This object is used for audio output. It is an abstract class.
 */

G_DEFINE_ABSTRACT_TYPE (SwfdecAudio, swfdec_audio, G_TYPE_OBJECT)

static void
swfdec_audio_dispose (GObject *object)
{
  SwfdecAudio *audio = SWFDEC_AUDIO (object);

  g_assert (audio->player == NULL);

  G_OBJECT_CLASS (swfdec_audio_parent_class)->dispose (object);
}

static void
swfdec_audio_class_init (SwfdecAudioClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_audio_dispose;
}

static void
swfdec_audio_init (SwfdecAudio *audio)
{
}

/**
 * swfdec_audio_add:
 * @audio: audio to add
 * @player: a #SwfdecPlayer to attach to or NULL
 *
 * Registers a new audio object for playback in @player. If player is %NULL,
 * this function does nothing.
 * The starting point of the audio stream will be equivalent the player's time.
 **/
void
swfdec_audio_add (SwfdecAudio *audio, SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_AUDIO (audio));
  g_return_if_fail (audio->player == NULL);
  if (player == NULL)
    return;
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  audio->player = player;
  player->audio = g_list_append (player->audio, audio);
  SWFDEC_INFO ("adding %s %p", G_OBJECT_TYPE_NAME (audio), audio);
  if (player->audio_skip) {
    /* i wonder if removing a just added audio causes issues */
    if (swfdec_audio_iterate (audio, player->audio_skip) == 0)
      swfdec_audio_remove (audio);
  }
}

void	
swfdec_audio_remove (SwfdecAudio *audio)
{
  g_return_if_fail (SWFDEC_IS_AUDIO (audio));

  if (audio->player != NULL) {
    SWFDEC_INFO ("removing %s %p", G_OBJECT_TYPE_NAME (audio), audio);
    audio->player->audio = g_list_remove (audio->player->audio, audio);
    if (audio->added) {
      g_signal_emit_by_name (audio->player, "audio-removed", audio);
      audio->added = FALSE;
    }
    audio->player = NULL;
  }
  g_object_unref (audio);
}

/**
 * swfdec_audio_iterate:
 * @audio: the #SwfdecAudio to iterate
 * @n_samples: number of samples to remove
 *
 * Iterates the @audio. Iterating means discarding the first @n_samples
 * samples of the audio stream.
 *
 * Returns: maximum number of remaining frames. If G_MAXUINT is returned,
 *          then the number of frames isn't known yet.
 **/
guint
swfdec_audio_iterate (SwfdecAudio *audio, guint n_samples)
{
  SwfdecAudioClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AUDIO (audio), 0);
  g_return_val_if_fail (n_samples > 0, 0);

  klass = SWFDEC_AUDIO_GET_CLASS (audio);
  g_assert (klass->iterate);
  return klass->iterate (audio, n_samples);
}

/**
 * swfdec_audio_render:
 * @audio: a #SwfdecAudio
 * @dest: memory area to render to
 * @start_offset: offset in samples at which to start rendering. The offset is 
 *		  calculated relative to the last iteration, so the value set 
 *		  by swfdec_player_set_audio_advance() is ignored.
 * @n_samples: amount of samples to render.
 *
 * Renders the samples from @audio into the area pointed to by @dest. The data 
 * is added to @dest, so you probably want to initialize @dest to silence 
 * before calling this function.
 **/
void
swfdec_audio_render (SwfdecAudio *audio, gint16 *dest, 
    guint start_offset, guint n_samples)
{
  SwfdecAudioClass *klass;

  g_return_if_fail (SWFDEC_IS_AUDIO (audio));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (n_samples > 0);

  klass = SWFDEC_AUDIO_GET_CLASS (audio);
  klass->render (audio, dest, start_offset, n_samples);
}

/**
 * swfdec_player_render_audio:
 * @player: a #SwfdecPlayer
 * @dest: location to add audio signal to. The audio signal will be in 
 *        44100kHz signed 16bit stereo.
 * @start_offset: offset in samples at which to start rendering. The offset is 
 *		  calculated relative to the last iteration, so the value set 
 *		  by swfdec_player_set_audio_advance() is ignored.
 * @n_samples: amount of samples to render.
 *
 * Renders the data for this frame into the given location. The data is added to @dest, 
 * so you probably want to initialize @dest to silence before calling this function.
 **/
void 
swfdec_player_render_audio (SwfdecPlayer *player, gint16* dest, 
    guint start_offset, guint n_samples)
{
  GList *walk;
  SwfdecAudio *audio;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (n_samples > 0);

  SWFDEC_LOG ("rendering offset %u, samples %u", start_offset, n_samples);
  for (walk = player->audio; walk; walk = walk->next) {
    audio = walk->data;
    swfdec_audio_render (audio, dest, start_offset, n_samples);
  }
}

