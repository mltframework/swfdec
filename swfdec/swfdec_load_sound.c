/* Swfdec
 * Copyright (C) 2003, 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_load_sound.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_decoder.h"
#include "swfdec_audio_internal.h"
#include "swfdec_audio_load.h"
#include "swfdec_bits.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_sandbox.h"
#include "swfdec_sound_provider.h"
#include "swfdec_stream_target.h"

/*** SWFDEC_SOUND_PROVIDER ***/

static void
swfdec_load_sound_sound_provider_start (SwfdecSoundProvider *provider,
    SwfdecActor *actor, gsize samples_offset, guint loops)
{
  SwfdecLoadSound *sound = SWFDEC_LOAD_SOUND (provider);

  if (sound->audio) {
    swfdec_audio_remove (sound->audio);
    g_object_unref (sound->audio);
  }
  if (samples_offset > 0 || loops > 1) {
    SWFDEC_FIXME ("implement starting at offset %"G_GSIZE_FORMAT" with %u loops",
	samples_offset, loops);
  }
  sound->audio = swfdec_audio_load_new (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (provider)->context), sound);
  swfdec_audio_set_matrix (sound->audio, &sound->sound_matrix);
}

static void
swfdec_load_sound_sound_provider_stop (SwfdecSoundProvider *provider, SwfdecActor *actor)
{
  SwfdecLoadSound *sound = SWFDEC_LOAD_SOUND (provider);

  if (sound->audio == NULL)
    return;

  swfdec_audio_set_matrix (sound->audio, NULL);
  swfdec_audio_remove (sound->audio);
  g_object_unref (sound->audio);
  sound->audio = NULL;
}

static void
swfdec_load_sound_sound_provider_init (SwfdecSoundProviderInterface *iface)
{
  iface->start = swfdec_load_sound_sound_provider_start;
  iface->stop = swfdec_load_sound_sound_provider_stop;
}

/*** SWFDEC_STREAM_TARGET ***/

static SwfdecPlayer *
swfdec_load_sound_stream_target_get_player (SwfdecStreamTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_LOAD_SOUND (target)->target->context);
}

static gboolean
swfdec_load_sound_mp3_parse_id3v2 (SwfdecLoadSound *sound, SwfdecBufferQueue *queue)
{
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint size;
  gboolean footer;

  buffer = swfdec_buffer_queue_peek (queue, 10);
  if (buffer == NULL)
    return FALSE;
  swfdec_bits_init (&bits, buffer);
  if (swfdec_bits_get_u8 (&bits) != 'I' ||
      swfdec_bits_get_u8 (&bits) != 'D' ||
      swfdec_bits_get_u8 (&bits) != '3')
    goto error;
  /* version = */ swfdec_bits_get_u16 (&bits);
  /* flags = */ swfdec_bits_getbits (&bits, 3);
  footer = swfdec_bits_getbit (&bits);
  /* reserved = */ swfdec_bits_getbits (&bits, 4);
  size = swfdec_bits_get_bu32 (&bits);
  if (size & 0x80808080)
    goto error;
  size = ((size & 0xFF000000) >> 3) |
    ((size & 0xFF0000) >> 2) |
    ((size & 0xFF00) >> 1) | (size & 0xFF);
  swfdec_buffer_unref (buffer);

  buffer = swfdec_buffer_queue_pull (queue, 10 + size + (footer ? 10 : 0));
  if (buffer == NULL)
    return FALSE;
  SWFDEC_FIXME ("implement ID3v2 parsing");
  SWFDEC_LOG ("%"G_GSIZE_FORMAT" bytes ID3v2", buffer->length);
  swfdec_buffer_unref (buffer);
  return TRUE;

error:
  swfdec_buffer_unref (buffer);
  swfdec_buffer_queue_flush (queue, 1);
  return TRUE;
}

static gboolean
swfdec_load_sound_mp3_parse_id3v1 (SwfdecLoadSound *sound, SwfdecBufferQueue *queue)
{
  SwfdecBuffer *buffer;
  
  buffer = swfdec_buffer_queue_pull (queue, 128);
  if (buffer == NULL)
    return FALSE;

  if (buffer->data[0] != 'T' ||
      buffer->data[1] != 'A' ||
      buffer->data[2] != 'G') {
    swfdec_buffer_unref (buffer);
    swfdec_buffer_queue_flush (queue, 1);
    return TRUE;
  }
  SWFDEC_FIXME ("implement ID3v1 parsing");
  swfdec_buffer_unref (buffer);
  return TRUE;
}

static gboolean
swfdec_load_sound_mp3_parse_frame (SwfdecLoadSound *sound, SwfdecBufferQueue *queue)
{
  static const guint mp3types_bitrates[2][3][16] = { 
    { {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, },
      {0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, },
      {0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, }},
    { {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, },
      {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, },
      {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, }}
  };
  static const guint mp3types_freqs[3][3] = { 
    { 11025, 12000,  8000 },
    { 22050, 24000, 16000 },
    { 44100, 48000, 32000 }
  };

  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint version, layer, bitrate, samplerate, length, channels;

  buffer = swfdec_buffer_queue_peek (queue, 4);
  if (buffer == NULL)
    return FALSE;

  swfdec_bits_init (&bits, buffer);
  if (swfdec_bits_getbits (&bits, 11) != 0x7FF)
    goto error;

  version = swfdec_bits_getbits (&bits, 2);
  if (version == 1)
    goto error;
  layer = 4 - swfdec_bits_getbits (&bits, 2);
  if (layer == 4)
    goto error;
  /* has_crc = */ swfdec_bits_getbit (&bits);
  bitrate = swfdec_bits_getbits (&bits, 4);
  if (bitrate == 0 || bitrate == 15) {
    if (bitrate == 0) {
      SWFDEC_FIXME ("need to support free frame length?");
    }
    goto error;
  }
  samplerate = swfdec_bits_getbits (&bits, 2);
  if (samplerate == 3)
    goto error;
  length = swfdec_bits_getbits (&bits, 1);
  /* unused = */ swfdec_bits_getbits (&bits, 1);
  channels = swfdec_bits_getbits (&bits, 2) == 3 ? 1 : 2;

  samplerate = mp3types_freqs[version > 0 ? version - 1 : 0][samplerate];
  bitrate = mp3types_bitrates[version == 3 ? 0 : 1][layer - 1][bitrate];
  if (layer == 1) {
    length = ((12000 * bitrate / samplerate) + length) * 4;
  } else {
    length += ((layer == 3 && version != 3) ? 72000 : 144000) 
      * bitrate / samplerate;
  }
  swfdec_buffer_unref (buffer);

  SWFDEC_LOG ("adding %u bytes mp3 frame", length);
  buffer = swfdec_buffer_queue_pull (queue, length);
  if (buffer == NULL)
    return FALSE;

  g_ptr_array_add (sound->frames, buffer);
  return TRUE;

error:
  swfdec_buffer_unref (buffer);
  swfdec_buffer_queue_flush (queue, 1);
  return TRUE;
}

static gboolean
swfdec_load_sound_stream_target_parse (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecLoadSound *sound = SWFDEC_LOAD_SOUND (target);
  SwfdecBufferQueue *queue;
  SwfdecBuffer *buffer;
  guint i;
  gboolean go_on = TRUE;

  /* decode MP3 into frames, ID3 tags and crap */
  queue = swfdec_stream_get_queue (stream);
  do {
    /* sync */
    buffer = swfdec_buffer_queue_peek_buffer (queue);
    if (buffer == NULL)
      break;
    for (i = 0; i < buffer->length; i++) {
      if (buffer->data[i] == 'I' || buffer->data[i] == 'T' || buffer->data[i] == 0xFF)
	break;
    }
    if (i) {
      SWFDEC_LOG ("sync: flushing %u bytes", i);
    }
    swfdec_buffer_queue_flush (queue, i);
    if (i == buffer->length) {
      swfdec_buffer_unref (buffer);
      continue;
    }
    /* parse data */
    switch (buffer->data[i]) {
      case 'I':
	/* ID3v2 */
	go_on = swfdec_load_sound_mp3_parse_id3v2 (sound, queue);
	break;
      case 'T':
	/* ID3v1 */
	go_on = swfdec_load_sound_mp3_parse_id3v1 (sound, queue);
	break;
      case 0xFF:
	/* MP3 frame */
	go_on = swfdec_load_sound_mp3_parse_frame (sound, queue);
	break;
      default:
	/* skip - and yes, the continue refers to the for loop */
	continue;
    }
    swfdec_buffer_unref (buffer);
  } while (go_on);
  return FALSE;
}

static void
swfdec_load_sound_stream_target_error (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecLoadSound *sound = SWFDEC_LOAD_SOUND (target);
  SwfdecAsValue val;

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, FALSE);
  swfdec_sandbox_use (sound->sandbox);
  swfdec_as_object_call (sound->target, SWFDEC_AS_STR_onLoad, 1, &val, NULL);
  swfdec_sandbox_unuse (sound->sandbox);

  swfdec_stream_set_target (stream, NULL);
  g_object_unref (stream);
  sound->stream = NULL;
}

static void
swfdec_load_sound_stream_target_close (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecLoadSound *sound = SWFDEC_LOAD_SOUND (target);
  SwfdecAsValue val;

  swfdec_stream_set_target (stream, NULL);
  g_object_unref (stream);
  sound->stream = NULL;

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, TRUE);
  swfdec_sandbox_use (sound->sandbox);
  swfdec_as_object_call (sound->target, SWFDEC_AS_STR_onLoad, 1, &val, NULL);
  swfdec_sandbox_unuse (sound->sandbox);
}

static void
swfdec_load_sound_stream_target_init (SwfdecStreamTargetInterface *iface)
{
  iface->get_player = swfdec_load_sound_stream_target_get_player;
  iface->parse = swfdec_load_sound_stream_target_parse;
  iface->close = swfdec_load_sound_stream_target_close;
  iface->error = swfdec_load_sound_stream_target_error;
}

/*** SWFDEC_LOAD_SOUND ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecLoadSound, swfdec_load_sound, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_STREAM_TARGET, swfdec_load_sound_stream_target_init);
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_SOUND_PROVIDER, swfdec_load_sound_sound_provider_init))

static void
swfdec_load_sound_dispose (GObject *object)
{
  SwfdecLoadSound *sound = SWFDEC_LOAD_SOUND (object);

  g_ptr_array_foreach (sound->frames, (GFunc) swfdec_buffer_unref, NULL);
  g_ptr_array_free (sound->frames, TRUE);
  if (sound->stream) {
    swfdec_stream_set_target (sound->stream, NULL);
    g_object_unref (sound->stream);
    sound->stream = NULL;
  }
  g_free (sound->url);
  if (sound->audio) {
    swfdec_audio_set_matrix (sound->audio, NULL);
    swfdec_audio_remove (sound->audio);
    g_object_unref (sound->audio);
    sound->audio = NULL;
  }

  G_OBJECT_CLASS (swfdec_load_sound_parent_class)->dispose (object);
}

static void
swfdec_load_sound_class_init (SwfdecLoadSoundClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_load_sound_dispose;
}

static void
swfdec_load_sound_init (SwfdecLoadSound *sound)
{
  sound->frames = g_ptr_array_new ();

  swfdec_sound_matrix_init_identity (&sound->sound_matrix);
}

static void
swfdec_load_sound_load (SwfdecPlayer *player, gboolean allow, gpointer data)
{
  SwfdecLoadSound *sound = data;

  if (!allow) {
    SwfdecAsValue val;

    SWFDEC_WARNING ("SECURITY: no access to %s from Sound.loadSound",
	sound->url);
    SWFDEC_AS_VALUE_SET_BOOLEAN (&val, FALSE);
    return;
  }

  sound->stream = SWFDEC_STREAM (swfdec_player_load (player, sound->url, NULL));
  swfdec_stream_set_target (sound->stream, SWFDEC_STREAM_TARGET (sound));
}

SwfdecLoadSound *
swfdec_load_sound_new (SwfdecAsObject *target, const char *url)
{
  SwfdecLoadSound *sound;
  char *missing;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  sound = g_object_new (SWFDEC_TYPE_LOAD_SOUND, NULL);
  sound->target = target;
  sound->sandbox = SWFDEC_SANDBOX (target->context->global);
  sound->url = g_strdup (url);
  swfdec_player_load_default (SWFDEC_PLAYER (target->context), url, 
      swfdec_load_sound_load, sound);
  /* tell missing plugins stuff we want MP3 */
  missing = NULL;
  swfdec_audio_decoder_prepare (SWFDEC_AUDIO_CODEC_MP3, 
      swfdec_audio_format_new (44100, 2, TRUE), &missing);
  if (missing) {
    swfdec_player_add_missing_plugin (SWFDEC_PLAYER (target->context),
	missing);
    g_free (missing);
  }

  return sound;
}

