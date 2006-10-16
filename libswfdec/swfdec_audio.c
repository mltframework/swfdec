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
#include "swfdec_sound.h"
#include "swfdec_sprite.h"

/*** GENERAL STUFF ***/

static SwfdecAudio *
swfdec_player_audio_new (SwfdecPlayer *player)
{
  guint i;
  for (i = 0; i < player->audio->len; i++) {
    SwfdecAudio *audio = &g_array_index (player->audio, SwfdecAudio, i);
    if (audio->type == SWFDEC_AUDIO_UNUSED)
      return audio;
  }
  g_array_set_size (player->audio, player->audio->len + 1);
  return &g_array_index (player->audio, SwfdecAudio, player->audio->len - 1);
}

/*** AUDIO STREAMS ***/

typedef struct {
  SwfdecSpriteFrame * 	frame;		/* NULL for silence */
  guint			skip;		/* number of samples to skip */
  guint			n_samples;	/* number of samples in this frame */
  SwfdecBuffer *	decoded;	/* decoded samples */
} StreamEntry;

static StreamEntry *
stream_entry_new_for_frame (SwfdecAudioStream *stream, guint frame_id)
{
  SwfdecSpriteFrame *frame = &stream->sprite->frames[frame_id];
  StreamEntry *entry;

  /* FIXME: someone figure out how "no new sound in some frames" is handled */
  entry = swfdec_ring_buffer_push (stream->playback_queue);
  if (entry == NULL) {
    swfdec_ring_buffer_set_size (stream->playback_queue,
	swfdec_ring_buffer_get_size (stream->playback_queue) + 8);
    entry = swfdec_ring_buffer_push (stream->playback_queue);
    g_assert (entry);
  }
  if (frame->sound_block == NULL) {
    entry->frame = NULL;
  } else {
    entry->frame = frame;
  }
  entry->n_samples = frame->sound_samples;
  entry->skip = 0;
  entry->decoded = NULL;
  stream->playback_samples += entry->n_samples;

  return entry;
}

static void
swfdec_audio_stream_ensure_size (SwfdecAudioStream *stream, guint n_samples)
{
  guint last_frame, last_samples;
  SwfdecSpriteFrame *frame;

  g_assert (!stream->disabled);
  last_frame = stream->current_frame;
  last_samples = stream->playback_samples;

  SWFDEC_LOG ("ensuring %u samples", n_samples);
  while (stream->playback_samples < n_samples) {
    frame = &stream->sprite->frames[stream->current_frame];
    if (frame->sound_head != stream->sound) {
      stream->disabled = TRUE;
      SWFDEC_WARNING ("sound head change!");
      return;
    }
    stream_entry_new_for_frame (stream, stream->current_frame);
    if (stream->sprite->parse_frame < stream->sprite->n_frames)
      SWFDEC_WARNING ("FIXME: sound for partially loaded frames not really implemented (lalala)");
    stream->current_frame = swfdec_sprite_get_next_frame (stream->sprite, stream->current_frame);
    if (last_frame == stream->current_frame &&
	last_samples == stream->playback_samples) {
      SWFDEC_ERROR ("no samples found in the whole stream");
      stream->disabled = TRUE;
      return;
    }
  }
  SWFDEC_LOG ("  got %u samples", stream->playback_samples);
  return;
}

static void
swfdec_audio_stream_pop (SwfdecAudioStream *stream)
{
  StreamEntry *entry = swfdec_ring_buffer_pop (stream->playback_queue);

  g_assert (entry);
  if (entry->decoded)
    swfdec_buffer_unref (entry->decoded);
}

static void
swfdec_audio_stream_flush (SwfdecAudioStream *stream, guint n_samples)
{
  if (n_samples > stream->playback_samples)
    n_samples = stream->playback_samples;

  SWFDEC_LOG ("flushing %u of %u samples\n", n_samples, stream->playback_samples);
  stream->playback_samples -= n_samples;
  while (n_samples) {
    StreamEntry *entry = swfdec_ring_buffer_peek_nth (stream->playback_queue, 0);
    if (n_samples >= entry->n_samples - entry->skip) {
      n_samples -= entry->n_samples - entry->skip;
      swfdec_audio_stream_pop (stream);
    } else {
      entry->skip += n_samples;
      n_samples = 0;
    }
  }
}

guint
swfdec_audio_stream_new (SwfdecPlayer *player, SwfdecSprite *sprite, guint start_frame)
{
  SwfdecSpriteFrame *frame;
  StreamEntry *entry;
  SwfdecAudioStream *stream = (SwfdecAudioStream *) swfdec_player_audio_new (player);

  SWFDEC_DEBUG ("new audio stream for sprite %d, starting at %u", 
      SWFDEC_CHARACTER (sprite)->id, start_frame);
  stream->type = SWFDEC_AUDIO_STREAM;
  stream->sprite = sprite;
  stream->playback_samples = 0;
  stream->playback_queue = swfdec_ring_buffer_new_for_type (StreamEntry, 16);
  stream->skip = player->samples_latency;
  frame = &sprite->frames[start_frame];
  g_assert (frame->sound_head);
  stream->sound = frame->sound_head;
  stream->disabled = FALSE;
  entry = stream_entry_new_for_frame (stream, start_frame);
  entry->skip = frame->sound_skip;
  g_assert (stream->playback_samples >= entry->skip);
  stream->playback_samples -= entry->skip;
  stream->current_frame = swfdec_sprite_get_next_frame (stream->sprite, start_frame);
  swfdec_audio_stream_ensure_size (stream, stream->skip + player->samples_this_frame);
  stream->decoder = swfdec_sound_init_decoder (stream->sound);
  swfdec_audio_stream_flush (stream, stream->skip);
  return (stream - (SwfdecAudioStream *) player->audio->data) + 1;
}

/* FIXME: name this unref? */
void
swfdec_audio_stream_stop (SwfdecPlayer *player, guint stream_id)
{
  SwfdecAudio *audio;

  g_assert (SWFDEC_IS_PLAYER (player));
  g_assert (stream_id <= player->audio->len);
  audio = &g_array_index (player->audio, SwfdecAudio, stream_id - 1);
  g_assert (audio->type == SWFDEC_AUDIO_STREAM);
  swfdec_audio_finish (audio);
}

/* return TRUE if done */
static gboolean
swfdec_audio_stream_iterate (SwfdecAudioStream *stream, guint remove, guint available)
{
  if (!stream->disabled) {
    if (available + remove > stream->skip)
      swfdec_audio_stream_ensure_size (stream, available + remove - stream->skip);
  }
  if (remove > stream->skip) {
    swfdec_audio_stream_flush (stream, remove);
    stream->skip = 0;
  } else {
    stream->skip -= remove;
  }
  SWFDEC_LOG ("having %u frames now%s - skip %u", stream->playback_samples, 
      stream->disabled ? " (disabled)" : "", stream->skip);
  return FALSE;
}

static void
swfdec_audio_stream_render (SwfdecAudioStream *stream, gint16* dest, guint start, guint n_samples)
{
  guint i;
  guint samples;

  if (stream->skip > start) {
    guint skip = stream->skip - start;
    if (skip > n_samples)
      return;
    start = 0;
    n_samples -= skip;
    dest += 2 * skip;
  } else {
    start -= stream->skip;
  }
  for (i = 0; i < swfdec_ring_buffer_get_n_elements (stream->playback_queue) && n_samples; i++) {
    StreamEntry *entry = swfdec_ring_buffer_peek_nth (stream->playback_queue, i);
    if (entry->decoded == NULL && entry->frame != NULL) {
      if (stream->disabled)
	return;
      entry->decoded = swfdec_sound_decode_buffer (stream->sound, stream->decoder, entry->frame->sound_block);
      if (entry->decoded == NULL) {
	SWFDEC_WARNING ("disabling stream because decoding failed");
	stream->disabled = TRUE;
	return;
      }
      if (entry->decoded->length != 4 * entry->n_samples) {
	SWFDEC_ERROR ("failed to decode sound correctly. Got %u samples instead of expected %u",
	    entry->decoded->length / 4, entry->n_samples);
	stream->disabled = TRUE;
	return;
      }
    }
    samples = entry->n_samples - entry->skip;
    if (samples > start) {
      guint skip = entry->skip + start;
      samples -= start;
      samples = MIN (samples, n_samples);
      if (entry->frame) {
	SWFDEC_LOG ("adding %u samples from %u (length %u)", samples, skip, entry->decoded->length);
	swfdec_sound_add (dest, (gint16 *) (entry->decoded->data + skip * 4), samples);
      }
      start = 0;
      n_samples -= samples;
      dest += samples * 2;
    } else {
      start -= MIN (start, samples);
    }
  }
}

/*** AUDIO EVENTS ***/

static void
swfdec_audio_event_stop (SwfdecPlayer *player, SwfdecSound *sound)
{
  guint i;

  for (i = 0; i < player->audio->len; i++) {
    SwfdecAudio *audio = &g_array_index (player->audio, SwfdecAudio, i);
    if (audio->type != SWFDEC_AUDIO_EVENT)
      continue;
    if (audio->event.sound == sound) {
      swfdec_audio_finish (audio);
      /* FIXME: stop one event or all? Currently we stop all */
    }
  }
}

static gboolean
swfdec_sound_is_playing (SwfdecPlayer *player, SwfdecSound *sound)
{
  guint i;

  for (i = 0; i < player->audio->len; i++) {
    SwfdecAudio *audio = &g_array_index (player->audio, SwfdecAudio, i);
    if (audio->type != SWFDEC_AUDIO_EVENT)
      continue;
    if (audio->event.sound == sound) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * swfdec_audio_event_init:
 * @player: a #SwfdecPlayer
 * @chunk: a sound chunk to start playing back
 *
 * Starts playback of the given sound chunk
 **/
void
swfdec_audio_event_init (SwfdecPlayer *player, SwfdecSoundChunk *chunk)
{
  SwfdecAudioEvent *event;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (chunk != NULL);

  if (chunk->stop) {
    swfdec_audio_event_stop (player, chunk->sound);
    return;
  }
  SWFDEC_LOG ("adding sound %d to playing sounds", SWFDEC_CHARACTER (chunk->sound)->id);
  if (chunk->no_restart &&
      swfdec_sound_is_playing (player, chunk->sound)) {
    SWFDEC_DEBUG ("sound %d is already playing, so not adding it", 
	SWFDEC_CHARACTER (chunk->sound)->id);
    return;
  }
  event = (SwfdecAudioEvent *) swfdec_player_audio_new (player);
  event->type = SWFDEC_AUDIO_EVENT;
  event->sound = chunk->sound;
  event->chunk = chunk;
  event->offset = chunk->start_sample;
  event->loop = 0;
  event->skip = player->samples_latency;
  SWFDEC_DEBUG ("playing sound %d from offset %d now", SWFDEC_CHARACTER (chunk->sound)->id,
      chunk->start_sample);
}

/* return TRUE if done */
static gboolean
swfdec_audio_event_iterate (SwfdecAudioEvent *stream, guint n_samples, guint new_samples)
{
  SwfdecSoundChunk *chunk = stream->chunk;

  /* FIXME: chunk offsets are absolute, not per frame, so this function does crap */
  if (stream->skip >= n_samples) {
    stream->skip -= n_samples;
    return TRUE;
  }
  n_samples -= stream->skip;
  stream->skip = 0;
  stream->offset += n_samples;
  while (stream->offset >= chunk->stop_sample) {
    stream->offset -= chunk->stop_sample + chunk->start_sample;
    stream->loop++;
    if (stream->loop >= chunk->loop_count)
      return TRUE;
  }
  return FALSE;
}

void
swfdec_audio_finish (SwfdecAudio *audio)
{
  switch (audio->type) {
    case SWFDEC_AUDIO_UNUSED:
    case SWFDEC_AUDIO_EVENT:
      break;
    case SWFDEC_AUDIO_STREAM:
      while (swfdec_ring_buffer_get_n_elements (audio->stream.playback_queue) > 0)
	swfdec_audio_stream_pop (&audio->stream);
      swfdec_sound_finish_decoder (audio->stream.sound, audio->stream.decoder);
      swfdec_ring_buffer_free (audio->stream.playback_queue);
      break;
    default: 
      g_assert_not_reached ();
  }
  audio->type = SWFDEC_AUDIO_UNUSED;
}

static void
swfdec_player_do_iterate_audio (SwfdecPlayer *player, guint remove)
{
  SwfdecAudio *audio;
  unsigned int i;
  gboolean ret;

  for (i = 0; i < player->audio->len; i++) {
    audio = &g_array_index (player->audio, SwfdecAudio, i);
    if (audio->type == SWFDEC_AUDIO_UNUSED) {
      ret = FALSE;
    } else if (audio->type == SWFDEC_AUDIO_EVENT) {
      ret = swfdec_audio_event_iterate (&audio->event, remove, 
	  player->samples_this_frame + player->samples_latency);
    } else {
      ret = swfdec_audio_stream_iterate (&audio->stream, remove, 
	  player->samples_this_frame + player->samples_latency);
    }
    if (ret)
      swfdec_audio_finish (audio);
  }
}

void
swfdec_player_iterate_audio (SwfdecPlayer *player)
{
  guint samples_last_frame;

  /* iterate all playing sounds */
  samples_last_frame = player->samples_this_frame;
  /* get new sample count */
  player->samples_this_frame = 44100 * 256 / player->rate;
  player->samples_overhead_left += player->samples_overhead;
  player->samples_overhead_left %= (44100 * 256);
  if (player->samples_overhead_left <= player->samples_overhead_left)
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
 * swfdec_player_get_latency:
 * @player: a #SwfdecPlayer
 *
 * Queries the latency of the audio stream. See swfdec_decoder_set_latency()
 * for details.
 *
 * Returns: the current latency in samples
 **/
guint
swfdec_player_get_latency (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);

  return player->samples_latency;
}

/**
 * swfdec_player_set_latency:
 * @player: a #SwfdecPlayer
 * @samples: latency in samples to use. 44100 would be one second.
 *
 * Sets the latency in @player to @samples. The latency determines how soon
 * sound events start playing. This function is intended to be set to the
 * buffer size of sound cards so that the application can fill the buffer but
 * still allow all events to be played back. Note that if you set this value
 * too high, the sound for clicking a button may start playing far too late.
 * Audio streams are still played back synchronized.
 **/
void
swfdec_player_set_latency (SwfdecPlayer *player, guint samples)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  if (player->samples_latency < samples) {
    player->samples_latency = samples;
    swfdec_player_do_iterate_audio (player, 0);
  } else {
    player->samples_latency = samples;
  }
}

static void
swfdec_audio_event_render (SwfdecAudioEvent *stream, gint16* dest, guint start, guint n_samples)
{
  const guint16 *volume = NULL;
  guint end, amount, i = 0;
  guint offset = stream->offset;
  guint loop = stream->loop;
  SwfdecSoundChunk *chunk = stream->chunk;

  n_samples += start;
  if (stream->skip >= n_samples)
    return;
  n_samples -= stream->skip;
  start -= MIN (stream->skip, start);
  while (n_samples > 0) {
    if (offset >= chunk->stop_sample) {
      offset = chunk->start_sample;
      i = 0;
      volume = NULL;
      loop++;
      if (loop >= chunk->loop_count)
	return;
    }
    if (chunk->n_envelopes) {
      while (i < chunk->n_envelopes && chunk->envelope[i].offset <= offset) {
	volume = chunk->envelope[i].volume;
	i++;
      }
      if (i < chunk->n_envelopes)
	end = chunk->envelope[i].offset;
      else
	end = chunk->stop_sample;
    } else {
      end = chunk->stop_sample;
    }
    amount = MIN (n_samples, end - offset);
    if (start >= amount) {
      start -= amount;
    } else {
      amount -= start;
      n_samples -= start;
      offset += start;
      swfdec_sound_render (chunk->sound, dest, offset, amount, volume);
    }
    n_samples -= amount;
    offset += amount;
  }
}

/**
 * swfdec_player_render_audio:
 * @player: a #SwfdecPlayer
 * @dest: location to add audio signal to. The audio signal will be in 44100kHz signed
 *        16bit stereo.
 * @start_offset: offset in samples at which to start rendering
 * @n_samples: amount of samples to render. The sum of @start_offset and @n_samples 
 *	       must not be bigger than the sum of swfdec_decoder_get_latency() and 
 *	       swfdec_decoder_get_audio_samples()
 *
 * Renders the data for this frame into the given location. The data is added to @dest, 
 * so you probably want to initialize @dest to silence before calling this function.
 **/
void 
swfdec_player_render_audio (SwfdecPlayer *player, gint16* dest, 
    guint start_offset, guint n_samples)
{
  SwfdecAudio *audio;
  guint i;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (n_samples > 0);
  g_return_if_fail (start_offset + n_samples <= player->samples_latency + player->samples_this_frame);

  for (i = 0; i < player->audio->len; i++) {
    audio = &g_array_index (player->audio, SwfdecAudio, i);
    if (audio->type == SWFDEC_AUDIO_UNUSED)
      continue;
    if (audio->type == SWFDEC_AUDIO_EVENT) {
      swfdec_audio_event_render (&audio->event, dest, start_offset, n_samples);
    } else {
      swfdec_audio_stream_render (&audio->stream, dest, start_offset, n_samples);
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

