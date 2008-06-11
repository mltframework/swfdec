/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_actor.h"
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

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_audio_dispose (GObject *object)
{
  SwfdecAudio *audio = SWFDEC_AUDIO (object);

  g_assert (audio->actor == NULL);
  g_assert (audio->player == NULL);

  G_OBJECT_CLASS (swfdec_audio_parent_class)->dispose (object);
}

static void
swfdec_audio_class_init (SwfdecAudioClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * SwfdecAudio::changed:
   * @audio: the #SwfdecAudio affected
   *
   * This signal is emitted whenever the data of the @audio changed and cached
   * data should be rerendered. This happens for example when the volume of the
   * audio is changed by the Flash file.
   */
  signals[CHANGED] = g_signal_new ("changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

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
  SwfdecPlayerPrivate *priv;

  g_return_if_fail (SWFDEC_IS_AUDIO (audio));
  g_return_if_fail (audio->player == NULL);
  if (player == NULL)
    return;
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  g_object_ref (audio);
  audio->player = player;
  priv = player->priv;
  priv->audio = g_list_append (priv->audio, audio);
  SWFDEC_INFO ("adding %s %p", G_OBJECT_TYPE_NAME (audio), audio);
}

void	
swfdec_audio_remove (SwfdecAudio *audio)
{
  g_return_if_fail (SWFDEC_IS_AUDIO (audio));

  if (audio->player != NULL) {
    SwfdecPlayerPrivate *priv = audio->player->priv;
    SWFDEC_INFO ("removing %s %p", G_OBJECT_TYPE_NAME (audio), audio);
    swfdec_audio_set_actor (audio, NULL);
    priv->audio = g_list_remove (priv->audio, audio);
    if (audio->added) {
      g_signal_emit_by_name (audio->player, "audio-removed", audio);
      audio->added = FALSE;
    }
    audio->player = NULL;
    g_object_unref (audio);
  }
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
gsize
swfdec_audio_iterate (SwfdecAudio *audio, guint n_samples)
{
  SwfdecAudioClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AUDIO (audio), 0);
  g_return_val_if_fail (n_samples > 0, 0);

  klass = SWFDEC_AUDIO_GET_CLASS (audio);
  g_assert (klass->iterate);
  return klass->iterate (audio, n_samples);
}

void
swfdec_audio_set_actor (SwfdecAudio *audio, SwfdecActor *actor)
{
  g_return_if_fail (SWFDEC_IS_AUDIO (audio));
  g_return_if_fail (audio->player != NULL);
  g_return_if_fail (actor == NULL || SWFDEC_IS_ACTOR (actor));

  if (actor) {
    g_object_ref (actor);
  }
  if (audio->actor) {
    g_object_unref (audio->actor);
  }
  audio->actor = actor;
}

/* FIXME: This function is pretty much a polling approach at sound matrix 
 * handling and it would be much nicer if we had a "changed" signal on the
 * matrices. But matrices can't emit signals...
 */
void
swfdec_audio_update_matrix (SwfdecAudio *audio)
{
  SwfdecSoundMatrix sound;

  g_return_if_fail (SWFDEC_IS_AUDIO (audio));

  if (audio->actor) {
    swfdec_sound_matrix_multiply (&sound, &audio->actor->sound_matrix,
	&audio->player->priv->sound_matrix);
  } else if (audio->player) {
    sound = audio->player->priv->sound_matrix;
  }
  if (swfdec_sound_matrix_is_equal (&sound, &audio->matrix))
    return;

  audio->matrix = sound;
  g_signal_emit (audio, signals[CHANGED], 0);
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
 * Renders the samples from @audio into the area pointed to by @dest. Existing
 * data in @dest is overwritten.
 *
 * Returns: The amount of samples actually rendered. Usually this number is 
 *          equal to @n_samples, but if you arrived at the end of stream or the
 *          stream is still loading, this number may be lower. It indicates 
 *          that no more samples are available.
 **/
gsize
swfdec_audio_render (SwfdecAudio *audio, gint16 *dest, 
    gsize start_offset, gsize n_samples)
{
  SwfdecAudioClass *klass;
  guint rendered;

  g_return_val_if_fail (SWFDEC_IS_AUDIO (audio), 0);
  g_return_val_if_fail (dest != NULL, 0);
  g_return_val_if_fail (n_samples > 0, 0);

  klass = SWFDEC_AUDIO_GET_CLASS (audio);
  rendered = klass->render (audio, dest, start_offset, n_samples);
  swfdec_sound_matrix_apply (&audio->matrix, dest, rendered);

  return rendered;
}

/*** SWFDEC_AUDIO_FORMAT ***/

/* SwfdecAudioFormat is represented in the least significant bits of a uint:
 * - the LSBit is 1 if it's 16bit audio, 0 for 8bit
 * - the next bit is 1 for stereo, 0 for mono
 * - the other two bits are for the rate, see swfdec_audio_format_new()
 * This is the same format the Flash file format uses to store audio formats.
 */

SwfdecAudioFormat
swfdec_audio_format_parse (SwfdecBits *bits)
{
  g_return_val_if_fail (bits != NULL, 0);

  return swfdec_bits_getbits (bits, 4);
}

SwfdecAudioFormat
swfdec_audio_format_new (guint rate, guint channels, gboolean is_16bit)
{
  SwfdecAudioFormat ret;

  g_return_val_if_fail (channels == 1 || channels == 2, 0);

  switch (rate) {
    case 44100:
      ret = 3 << 2; 
      break;
    case 22050:
      ret = 2 << 2; 
      break;
    case 11025:
      ret = 1 << 2; 
      break;
    case 5512:
      ret = 0 << 2; 
      break;
    default:
      g_return_val_if_reached (0);
      break;
  }
  if (is_16bit)
    ret |= 2;
  if (channels == 2)
    ret |= 1;

  return ret;
}

guint
swfdec_audio_format_get_channels (SwfdecAudioFormat format)
{
  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), 2);

  return (format & 0x1) + 1;
}

gboolean
swfdec_audio_format_is_16bit (SwfdecAudioFormat	format)
{
  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), TRUE);

  return format & 0x2 ? TRUE : FALSE;
}

guint
swfdec_audio_format_get_rate (SwfdecAudioFormat	format)
{
  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), 44100);

  return 44100 / swfdec_audio_format_get_granularity (format);
}

/**
 * swfdec_audio_format_get_granularity:
 * @format: an auio format
 *
 * The granularity is a Swfdec-specific name, describing how often a sample in
 * a 44100Hz audio stream is defined. So for example 44100Hz has a granularity 
 * of 1 and 11025Hz has a granularity of 4 (because only every fourth sample 
 * is defined).
 *
 * Returns: the granularity of the format
 **/
guint
swfdec_audio_format_get_granularity (SwfdecAudioFormat format)
{
  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), 1);

  return 1 << (3 - (format >> 2));
}

const char *
swfdec_audio_format_to_string (SwfdecAudioFormat format)
{
  static const char *names[] = {
    "8bit 5.5kHz mono",
    "8bit 5.5kHz stereo",
    "16bit 5.5kHz mono",
    "16bit 5.5kHz stereo",
    "8bit 11kHz mono",
    "8bit 11kHz stereo",
    "16bit 11kHz mono",
    "16bit 11kHz stereo",
    "8bit 22kHz mono",
    "8bit 22kHz stereo",
    "16bit 22kHz mono",
    "16bit 22kHz stereo",
    "8bit 44kHz mono",
    "8bit 44kHz stereo",
    "16bit 44kHz mono",
    "16bit 44kHz stereo"
  };
  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), "");

  return names[format];
}

/**
 * swfdec_audio_format_get_bytes_per_sample:
 * @format: audio format to check
 *
 * Computes the number of bytes required to store one sample of audio encoded
 * in @format.
 *
 * Returns: The number of bytes for one sample
 **/
guint
swfdec_audio_format_get_bytes_per_sample (SwfdecAudioFormat format)
{
  guint bps[4] = { 1, 2, 2, 4 };

  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), 1);

  return bps [format & 0x3];
}

