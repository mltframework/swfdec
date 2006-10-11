
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "swfdec_audio.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"
#include "swfdec_movieclip.h"

/*** GENERAL STUFF ***/

static SwfdecAudio *
swfdec_decoder_audio_new (SwfdecDecoder *dec)
{
  guint i;
  for (i = 0; i < dec->audio->len; i++) {
    SwfdecAudio *audio = &g_array_index (dec->audio, SwfdecAudio, i);
    if (audio->type == SWFDEC_AUDIO_UNUSED)
      return audio;
  }
  g_array_set_size (dec->audio, dec->audio->len + 1);
  return &g_array_index (dec->audio, SwfdecAudio, dec->audio->len - 1);
}

/*** AUDIO STREAMS ***/

typedef struct {
  SwfdecSpriteFrame * 	frame;		/* NULL for silence */
  guint			skip;		/* number of samples to skip */
  guint			n_samples;	/* number of samples in this frame */
  SwfdecBuffer *	decoded;	/* decoded samples */
} StreamEntry;

static StreamEntry *
stream_entry_new_for_frame (SwfdecSprite *sprite, guint frame_id)
{
  SwfdecSpriteFrame *frame = &sprite->frames[frame_id];
  StreamEntry *entry;

  /* FIXME: someone figure out how "no new sound in some frames" is handled */
  entry = g_slice_new (StreamEntry);
  if (frame->sound_samples == 0) {
    entry->frame = NULL;
    entry->n_samples = 44100 * 256 / SWFDEC_OBJECT (sprite)->decoder->rate;
  } else {
    entry->frame = frame;
    entry->n_samples = frame->sound_samples;
  }
  entry->skip = 0;
  entry->decoded = NULL;

  return entry;
}

static void
stream_entry_free (StreamEntry *entry)
{
  if (entry->decoded)
    swfdec_buffer_unref (entry->decoded);
  g_slice_free (StreamEntry, entry);
}

static void
swfdec_audio_stream_push_entry (SwfdecAudioStream *stream, StreamEntry *entry)
{
  stream->playback_samples += entry->n_samples - entry->skip;
  g_queue_push_tail (stream->playback_queue, entry);
}

static void
swfdec_audio_stream_ensure_size (SwfdecAudioStream *stream, guint n_samples)
{
  StreamEntry *entry;
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
    entry = stream_entry_new_for_frame (stream->sprite, stream->current_frame);
    if (entry)
      swfdec_audio_stream_push_entry (stream, entry);
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
  StreamEntry *entry = g_queue_pop_head (stream->playback_queue);

  g_assert (entry);
  stream_entry_free (entry);
}

static void
swfdec_audio_stream_flush (SwfdecAudioStream *stream, guint n_samples)
{
  if (n_samples > stream->playback_samples)
    n_samples = stream->playback_samples;

  SWFDEC_LOG ("flushing %u of %u samples\n", n_samples, stream->playback_samples);
  stream->playback_samples -= n_samples;
  while (n_samples) {
    StreamEntry *entry = g_queue_peek_head (stream->playback_queue);
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
swfdec_audio_stream_new (SwfdecSprite *sprite, guint start_frame)
{
  SwfdecSpriteFrame *frame;
  StreamEntry *entry;
  SwfdecDecoder *dec = SWFDEC_OBJECT (sprite)->decoder;
  SwfdecAudioStream *stream = (SwfdecAudioStream *) swfdec_decoder_audio_new (dec);

  stream->type = SWFDEC_AUDIO_STREAM;
  stream->sprite = sprite;
  stream->playback_samples = 0;
  stream->playback_queue = g_queue_new ();
  stream->skip = dec->samples_latency;
  frame = &sprite->frames[start_frame];
  g_assert (frame->sound_head);
  stream->sound = frame->sound_head;
  stream->disabled = FALSE;
  entry = stream_entry_new_for_frame (stream->sprite, start_frame);
  if (entry) {
    entry->skip = frame->sound_skip;
    swfdec_audio_stream_push_entry (stream, entry);
  }
  stream->current_frame = swfdec_sprite_get_next_frame (stream->sprite, start_frame);
  swfdec_audio_stream_ensure_size (stream, stream->skip + dec->samples_this_frame);
  stream->decoder = swfdec_sound_init_decoder (stream->sound);
  swfdec_audio_stream_flush (stream, stream->skip);
  return (stream - (SwfdecAudioStream *) dec->audio->data) + 1;
}

/* FIXME: name this unref? */
void
swfdec_audio_stream_stop (SwfdecDecoder *dec, guint stream_id)
{
  SwfdecAudio *audio;

  g_assert (SWFDEC_IS_DECODER (dec));
  g_assert (stream_id <= dec->audio->len);
  audio = &g_array_index (dec->audio, SwfdecAudio, stream_id - 1);
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
  GList *walk;
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
  for (walk = g_queue_peek_head_link (stream->playback_queue); walk && n_samples; walk = walk->next) {
    StreamEntry *entry = walk->data;
    if (entry->decoded == NULL && entry->frame != NULL) {
      if (stream->disabled)
	return;
      entry->decoded = swfdec_sound_decode_buffer (stream->sound, stream->decoder, entry->frame->sound_block);
      if (entry->decoded == NULL) {
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
swfdec_audio_event_stop (SwfdecDecoder *dec, SwfdecSound *sound)
{
  guint i;

  for (i = 0; i < dec->audio->len; i++) {
    SwfdecAudio *audio = &g_array_index (dec->audio, SwfdecAudio, i);
    if (audio->type != SWFDEC_AUDIO_EVENT)
      continue;
    if (audio->event.sound == sound) {
      swfdec_audio_finish (audio);
      return;
    }
  }
  SWFDEC_WARNING ("sound %d isn't playing, so not removed", SWFDEC_OBJECT (sound)->id);
}

static gboolean
swfdec_sound_is_playing (SwfdecDecoder *dec, SwfdecSound *sound)
{
  guint i;

  for (i = 0; i < dec->audio->len; i++) {
    SwfdecAudio *audio = &g_array_index (dec->audio, SwfdecAudio, i);
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
 * @dec: a #SwfdecDecoder
 * @chunk: a sound chunk to start playing back
 *
 * Starts playback of the given sound chunk
 **/
void
swfdec_audio_event_init (SwfdecDecoder *dec, SwfdecSoundChunk *chunk)
{
  SwfdecAudio *stream;

  g_return_if_fail (SWFDEC_IS_DECODER (dec));
  g_return_if_fail (chunk != NULL);

  if (chunk->stop) {
    swfdec_audio_event_stop (dec, chunk->sound);
    return;
  }
  SWFDEC_LOG ("adding sound %d to playing sounds", SWFDEC_OBJECT (chunk->sound)->id);
  if (chunk->no_restart &&
      swfdec_sound_is_playing (dec, chunk->sound)) {
    SWFDEC_DEBUG ("sound %d is already playing, so not adding it", 
	SWFDEC_OBJECT (chunk->sound)->id);
    return;
  }
  stream = swfdec_decoder_audio_new (dec);
  stream->type = SWFDEC_AUDIO_EVENT;
  stream->event.sound = chunk->sound;
  stream->event.chunk = chunk;
  stream->event.offset = chunk->start_sample;
  stream->event.loop = 0;
  stream->event.skip = dec->samples_latency;
  SWFDEC_DEBUG ("playing sound %d from offset %d now", SWFDEC_OBJECT (chunk->sound)->id,
      chunk->start_sample);
}

/* return TRUE if done */
static gboolean
swfdec_audio_event_iterate (SwfdecAudioEvent *stream, guint n_samples, guint new_samples)
{
  SwfdecSoundChunk *chunk = stream->chunk;

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
      while (!g_queue_is_empty (audio->stream.playback_queue))
	swfdec_audio_stream_pop (&audio->stream);
      swfdec_sound_finish_decoder (audio->stream.sound, audio->stream.decoder);
      break;
    default: 
      g_assert_not_reached ();
  }
  audio->type = SWFDEC_AUDIO_UNUSED;
}

void
swfdec_audio_iterate (SwfdecDecoder *dec)
{
  SwfdecAudio *audio;
  unsigned int i, samples_last_frame;
  gboolean ret;

  /* iterate all playing sounds */
  i = 0;
  samples_last_frame = dec->samples_this_frame;
  /* get new sample count */
  dec->samples_this_frame = 44100 * 256 / dec->rate;
  dec->samples_overhead_left += dec->samples_overhead;
  dec->samples_overhead_left %= (44100 * 256);
  if (dec->samples_overhead_left <= dec->samples_overhead_left)
    dec->samples_this_frame++;
  while (i < dec->audio->len) {
    audio = &g_array_index (dec->audio, SwfdecAudio, i);
    if (audio->type == SWFDEC_AUDIO_UNUSED) {
      ret = FALSE;
    } else if (audio->type == SWFDEC_AUDIO_EVENT) {
      ret = swfdec_audio_event_iterate (&audio->event, samples_last_frame, dec->samples_this_frame + dec->samples_latency);
    } else {
      ret = swfdec_audio_stream_iterate (&audio->stream, samples_last_frame, dec->samples_this_frame + dec->samples_latency);
    }
    if (ret) {
      swfdec_audio_finish (audio);
    } else {
      i++;
    }
  }
}

/**
 * swfdec_decoder_get_audio_samples:
 * @dec: a #SwfdecDecoder
 *
 * Gets the amount of audio samples to be played back in the current frame. The
 * amount of samples may differ by one between frames to work around rounding 
 * issues.
 *
 * Returns: amount of samples in the current frame.
 **/
guint
swfdec_decoder_get_audio_samples (SwfdecDecoder *dec)
{
  g_return_val_if_fail (SWFDEC_IS_DECODER (dec), 0);

  return dec->samples_this_frame;
}

guint
swfdec_decoder_get_latency (SwfdecDecoder *dec)
{
  g_return_val_if_fail (SWFDEC_IS_DECODER (dec), 0);

  return dec->samples_latency;
}

void
swfdec_decoder_set_latency (SwfdecDecoder *dec, guint samples)
{
  g_return_if_fail (SWFDEC_IS_DECODER (dec));

  dec->samples_latency = samples;
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
 * swfdec_decoder_render_audio:
 * @dec: a #SwfdecDecoder
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
swfdec_decoder_render_audio (SwfdecDecoder *dec, gint16* dest, 
    guint start_offset, guint n_samples)
{
  SwfdecAudio *audio;
  guint i;

  g_return_if_fail (SWFDEC_IS_DECODER (dec));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (n_samples > 0);
  g_return_if_fail (start_offset + n_samples <= dec->samples_latency + dec->samples_this_frame);

  for (i = 0; i < dec->audio->len; i++) {
    audio = &g_array_index (dec->audio, SwfdecAudio, i);
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
 * swfdec_decoder_render_audio_to_buffer:
 * @dec: a #SwfdecDecoder
 *
 * Renders the audio for this frame into a new buffer.
 *
 * Returns: a new #SwfdecBuffer fille with the audio for this frame.
 **/
SwfdecBuffer *
swfdec_decoder_render_audio_to_buffer (SwfdecDecoder *dec)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (SWFDEC_IS_DECODER (dec), NULL);

  buffer = swfdec_buffer_new ();
  buffer->length = dec->samples_this_frame * 4;
  buffer->data = g_malloc0 (buffer->length);
  swfdec_decoder_render_audio (dec, (gint16 *) buffer->data,
      dec->samples_latency, dec->samples_this_frame);

  return buffer;
}

