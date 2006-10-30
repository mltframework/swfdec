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
#include "swfdec_audio.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"


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
 * swfdec_audio_new:
 * @player: a #SwfdecPlayer
 * @type: type of audio to create
 *
 * Creates a new audio object and registers it for playbakc in @type.
 * The start offset of the audio stream will be equivalent to the point
 * set via swfdec_player_set_audio_advance().
 *
 * Returns: the new #SwfdecAudio object
 **/
SwfdecAudio *
swfdec_audio_new (SwfdecPlayer *player, GType type)
{
  SwfdecAudio *ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (g_type_is_a (type, SWFDEC_TYPE_AUDIO), NULL);

  ret = g_object_new (type, NULL);
  ret->player = player;
  player->audio = g_list_append (player->audio, ret);
  player->audio_changed = TRUE;
  ret->start_offset = player->samples_latency;

  return ret;
}

void	
swfdec_audio_remove (SwfdecAudio *audio)
{
  g_return_if_fail (SWFDEC_IS_AUDIO (audio));
  g_return_if_fail (audio->player != NULL);

  if (audio->player != NULL) {
    audio->player->audio = g_list_remove (audio->player->audio, audio);
    audio->player->audio_changed = TRUE;
    audio->player = NULL;
  }
  g_object_unref (audio);
}

static void
swfdec_player_do_iterate_audio (SwfdecPlayer *player, guint remove)
{
  GList *walk;
  SwfdecAudioClass *klass;
  SwfdecAudio *audio;

  walk = player->audio;
  while (walk) {
    audio = walk->data;
    walk = walk->next;
    klass = SWFDEC_AUDIO_GET_CLASS (audio);
    if (klass->iterate (audio, remove))
      swfdec_audio_remove (audio);
  }
}

void
swfdec_player_iterate_audio (SwfdecPlayer *player)
{
  guint samples_last_frame;

  g_assert (player->samples_latency >= player->samples_this_frame);
  player->samples_latency -= player->samples_this_frame;
  /* iterate all playing sounds */
  samples_last_frame = player->samples_this_frame;
  /* get new sample count */
  player->samples_this_frame = 44100 * 256 / player->rate;
  player->samples_overhead_left += player->samples_overhead;
  player->samples_overhead_left %= (44100 * 256);
  if (player->samples_overhead_left < player->samples_overhead_left)
    player->samples_this_frame++;
  swfdec_player_do_iterate_audio (player, samples_last_frame);
}

/**
 * swfdec_player_get_audio_samples:
 * @dec: a #SwfdecPlayer
 *
 * Gets the amount of audio samples to be played back in the current frame. The
 * amount of samples may differ by one between frames to work around rounding 
 * issues.
 *
 * Returns: amount of samples in the current frame.
 **/
guint
swfdec_player_get_audio_samples (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);

  return player->samples_this_frame;
}

/**
 * swfdec_player_get_audio_advance:
 * @player: a #SwfdecPlayer
 *
 * Queries the current latency of the audio stream. See swfdec_decoder_set_audio_advance()
 * for details.
 *
 * Returns: the current latency in samples
 **/
guint
swfdec_player_get_audio_advance (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);

  return player->samples_latency;
}

/**
 * swfdec_player_set_audio_advance:
 * @player: a #SwfdecPlayer
 * @samples: how many frames the audio is advanced since the last iteration in frames.
 *
 * Sets the current latency in @player to @samples. The latency determines how soon
 * sound events start playing. This function is intended to be set when iterating 
 * too late or when handling mouse events so that audio based on those events
 * are started at the right point. 
 * You can think of audio and video as two timelines that normally advance at the same 
 * speed. Since audio has a much higher granularity (44100th of a second vs 256th of a
 * second) and an iteration advances the video one time, this function is provided to
 * advance the audio seperately.
 * Note that when iterating the video "catches up" to the audio as the audio advance is
 * reduced by the amount of samples in the current frame. So if you want to use a 
 * constant advance, you'll have to call this function before every iteration with an 
 * updated value.
 **/
void
swfdec_player_set_audio_advance (SwfdecPlayer *player, guint samples)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  player->samples_latency = samples;
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
  SwfdecAudioClass *klass;
  SwfdecAudio *audio;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (n_samples > 0);

  for (walk = player->audio; walk; walk = walk->next) {
    audio = walk->data;
    klass = SWFDEC_AUDIO_GET_CLASS (audio);
    if (audio->start_offset) {
      if (audio->start_offset >= start_offset + n_samples)
	continue;
      if (audio->start_offset >= start_offset)
	klass->render (audio, dest + (audio->start_offset - start_offset) * 2, 0, 
	    n_samples + start_offset - audio->start_offset);
      klass->render (audio, dest, start_offset - audio->start_offset, n_samples);
    } else {
      klass->render (audio, dest, start_offset, n_samples);
    }
  }
}

/**
 * swfdec_player_render_audio_to_buffer:
 * @player: a #SwfdecPlayer
 *
 * Renders the audio for this frame into a new buffer. This is a simplification
 * for swfdec_player_render_audio().
 *
 * Returns: a new #SwfdecBuffer fille with the audio for this frame.
 **/
SwfdecBuffer *
swfdec_player_render_audio_to_buffer (SwfdecPlayer *player)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  buffer = swfdec_buffer_new ();
  buffer->length = player->samples_this_frame * 4;
  buffer->data = g_malloc0 (buffer->length);
  swfdec_player_render_audio (player, (gint16 *) buffer->data,
      player->samples_latency, player->samples_this_frame);

  return buffer;
}

