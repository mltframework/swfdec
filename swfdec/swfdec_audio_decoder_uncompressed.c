/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_audio_decoder_uncompressed.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

G_DEFINE_TYPE (SwfdecAudioDecoderUncompressed, swfdec_audio_decoder_uncompressed, SWFDEC_TYPE_AUDIO_DECODER)

static gboolean
swfdec_audio_decoder_uncompressed_prepare (guint codec, SwfdecAudioFormat format, char **missing)
{
  return codec == SWFDEC_AUDIO_CODEC_UNDEFINED ||
      codec == SWFDEC_AUDIO_CODEC_UNCOMPRESSED;
}

static SwfdecAudioDecoder *
swfdec_audio_decoder_uncompressed_create (guint codec, SwfdecAudioFormat format)
{
  if (codec != SWFDEC_AUDIO_CODEC_UNDEFINED &&
      codec != SWFDEC_AUDIO_CODEC_UNCOMPRESSED)
    return NULL;

  return g_object_new (SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED, NULL);
}

static SwfdecBuffer *
swfdec_audio_decoder_uncompressed_upscale (SwfdecBuffer *buffer, SwfdecAudioFormat format)
{
  guint channels = swfdec_audio_format_get_channels (format);
  guint granularity = swfdec_audio_format_get_granularity (format);
  SwfdecBuffer *ret;
  guint i, j, n_samples;
  gint16 *src, *dest;

  g_printerr ("buffer length %u, channels %u, granularity %u\n", buffer->length, channels, granularity);
  n_samples = buffer->length / 2 / channels;
  if (n_samples * 2 * channels != buffer->length) {
    SWFDEC_ERROR ("incorrect buffer size, %"G_GSIZE_FORMAT" bytes overhead",
	buffer->length - n_samples * 2 * channels);
  }
  ret = swfdec_buffer_new (n_samples * 4 * granularity);
  src = (gint16 *) buffer->data;
  dest = (gint16 *) ret->data;
  for (i = 0; i < n_samples; i++) {
    for (j = 0; j < granularity; j++) {
      *dest++ = src[0];
      *dest++ = src[channels - 1];
    }
    src += channels;
  }

  swfdec_buffer_unref (buffer);
  return ret;
}

static SwfdecBuffer *
swfdec_audio_decoder_uncompressed_decode_8bit (SwfdecBuffer *buffer)
{
  SwfdecBuffer *ret;
  gint16 *out;
  guint8 *in;
  guint i;

  ret = swfdec_buffer_new (buffer->length * 2);
  out = (gint16 *) (void *) ret->data;
  in = buffer->data;
  for (i = 0; i < buffer->length; i++) {
    *out = ((gint16) *in << 8) ^ (-1);
    out++;
    in++;
  }
  return ret;
}

static SwfdecBuffer *
swfdec_audio_decoder_uncompressed_decode_16bit (SwfdecBuffer *buffer)
{
  SwfdecBuffer *ret;
  gint16 *src, *dest;
  guint i;

  ret = swfdec_buffer_new (buffer->length);
  src = (gint16 *) buffer->data;
  dest = (gint16 *) ret->data;
  for (i = 0; i < buffer->length; i += 2) {
    *dest = GINT16_FROM_LE (*src);
    dest++;
    src++;
  }

  return ret;
}

static void
swfdec_audio_decoder_uncompressed_push (SwfdecAudioDecoder *decoder, 
    SwfdecBuffer *buffer)
{
  SwfdecBuffer *tmp;

  if (buffer == NULL)
    return;

  if (swfdec_audio_format_is_16bit (decoder->format))
    tmp = swfdec_audio_decoder_uncompressed_decode_16bit (buffer);
  else
    tmp = swfdec_audio_decoder_uncompressed_decode_8bit (buffer);

  tmp = swfdec_audio_decoder_uncompressed_upscale (tmp, decoder->format);
  swfdec_buffer_queue_push (SWFDEC_AUDIO_DECODER_UNCOMPRESSED (decoder)->queue,
      tmp);
}

static SwfdecBuffer *
swfdec_audio_decoder_uncompressed_pull (SwfdecAudioDecoder *decoder)
{
  SwfdecAudioDecoderUncompressed *dec = (SwfdecAudioDecoderUncompressed *) decoder;
  
  return swfdec_buffer_queue_pull_buffer (dec->queue);
}

static void
swfdec_audio_decoder_uncompressed_dispose (GObject *object)
{
  SwfdecAudioDecoderUncompressed *dec = (SwfdecAudioDecoderUncompressed *) object;

  swfdec_buffer_queue_unref (dec->queue);

  G_OBJECT_CLASS (swfdec_audio_decoder_uncompressed_parent_class)->dispose (object);
}

static void
swfdec_audio_decoder_uncompressed_class_init (SwfdecAudioDecoderUncompressedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioDecoderClass *decoder_class = SWFDEC_AUDIO_DECODER_CLASS (klass);

  object_class->dispose = swfdec_audio_decoder_uncompressed_dispose;

  decoder_class->prepare = swfdec_audio_decoder_uncompressed_prepare;
  decoder_class->create = swfdec_audio_decoder_uncompressed_create;
  decoder_class->pull = swfdec_audio_decoder_uncompressed_pull;
  decoder_class->push = swfdec_audio_decoder_uncompressed_push;
}

static void
swfdec_audio_decoder_uncompressed_init (SwfdecAudioDecoderUncompressed *dec)
{
  dec->queue = swfdec_buffer_queue_new ();
}

