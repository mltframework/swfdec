
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "swfdec_audio.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_sound.h"
#include "swfdec_movieclip.h"

static void
swfdec_audio_event_stop (SwfdecDecoder *dec, SwfdecSound *sound)
{
  guint i;

  for (i = 0; i < dec->audio_events->len; i++) {
    if (g_array_index (dec->audio_events, SwfdecAudioEvent, i).sound == sound) {
      g_array_remove_index_fast (dec->audio_events, i);
      return;
    }
  }
  SWFDEC_WARNING ("sound %d isn't playing, so not removed", SWFDEC_OBJECT (sound)->id);
}

static gboolean
swfdec_sound_is_playing (SwfdecDecoder *dec, SwfdecSound *sound)
{
  guint i;

  for (i = 0; i < dec->audio_events->len; i++) {
    if (g_array_index (dec->audio_events, SwfdecAudioEvent, i).sound == sound) {
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
  SwfdecAudioEvent stream;

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
  stream.sound = chunk->sound;
  stream.chunk = chunk;
  stream.offset = chunk->start_sample;
  stream.loop = 0;
  g_array_append_val (dec->audio_events, stream);
  g_print ("playing sound %d from offset %d now\n", SWFDEC_OBJECT (chunk->sound)->id,
      chunk->start_sample);
}

/* return TRUE if done */
static gboolean
swfdec_audio_event_iterate (SwfdecAudioEvent *stream, guint n_samples)
{
  SwfdecSoundChunk *chunk = stream->chunk;

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
swfdec_audio_iterate_start (SwfdecDecoder *dec)
{
  SwfdecAudioEvent *stream;
  unsigned int i;

  /* iterate all playing sounds */
  i = 0;
  while (i < dec->audio_events->len) {
    stream = &g_array_index (dec->audio_events, SwfdecAudioEvent, i);
    if (swfdec_audio_event_iterate (stream, dec->samples_this_frame)) {
      g_array_remove_index_fast (dec->audio_events, i);
    } else {
      i++;
    }
  }
}

void
swfdec_audio_iterate_finish (SwfdecDecoder *dec)
{
  /* get new sample count */
  dec->samples_this_frame = 44100 * 256 / dec->rate;
  dec->samples_overhead_left += dec->samples_overhead;
  dec->samples_overhead_left %= (44100 * 256);
  if (dec->samples_overhead_left <= dec->samples_overhead_left)
    dec->samples_this_frame++;
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

static void
swfdec_decoder_render_audio_event (SwfdecAudioEvent *stream, gint16* dest, guint n_samples)
{
  static const guint16 default_volume[2] = { G_MAXUINT16, G_MAXUINT16 };
  const guint16 *volume = default_volume;
  guint end, amount, i = 0;
  guint offset = stream->offset;
  guint loop = stream->loop;
  SwfdecSoundChunk *chunk = stream->chunk;

  while (n_samples > 0) {
    if (offset >= chunk->stop_sample) {
      offset = chunk->start_sample;
      i = 0;
      volume = default_volume;
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
    swfdec_sound_render (chunk->sound, dest, offset, amount, volume);
    n_samples -= amount;
    offset += amount;
  }
}

/**
 * swfdec_decoder_render_audio:
 * @dec: a #SwfdecDecoder
 * @dest: location to add audio signal to. The audio signal will be in 44100kHz signed
 *        16bit stereo.
 * @dest_size: amount of bytes to render. Must be divisable by 4 and not bigger 
 *             than the maximum amount of 
 *
 * Renders the data for this frame into the given location. The data is added, so
 * new buffers must be initialized to silence.
 **/
void 
swfdec_decoder_render_audio (SwfdecDecoder *dec, gint16* dest, guint dest_size)
{
  SwfdecAudioEvent *stream;
  guint i;

  g_return_if_fail (SWFDEC_IS_DECODER (dec));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (dest_size > 0 && dest_size <= dec->samples_this_frame * 4);
  g_return_if_fail (dest_size % 4 == 0);

  dest_size /= 4;
  for (i = 0; i < dec->audio_events->len; i++) {
    stream = &g_array_index (dec->audio_events, SwfdecAudioEvent, i);
    swfdec_decoder_render_audio_event (stream, dest, dest_size);
  }
  swfdec_movie_clip_render_audio (dec->root, dest, dest_size);
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
  swfdec_decoder_render_audio (dec, (gint16 *) buffer->data, buffer->length);

  return buffer;
}

