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

#include "swfdec_audio_decoder.h"
#include "swfdec_audio_decoder_adpcm.h"
#include "swfdec_audio_decoder_gst.h"
#include "swfdec_audio_decoder_uncompressed.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

G_DEFINE_TYPE (SwfdecAudioDecoder, swfdec_audio_decoder, G_TYPE_OBJECT)

static void
swfdec_audio_decoder_class_init (SwfdecAudioDecoderClass *klass)
{
}

static void
swfdec_audio_decoder_init (SwfdecAudioDecoder *audio_decoder)
{
}

static SwfdecAudioDecoder *
swfdec_audio_decoder_builtin_new (guint codec, SwfdecAudioFormat format)
{
  switch (format) {
    case SWFDEC_AUDIO_CODEC_UNDEFINED:
    case SWFDEC_AUDIO_CODEC_UNCOMPRESSED:
      return g_object_new (SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED, NULL);
    case SWFDEC_AUDIO_CODEC_ADPCM:
      return g_object_new (SWFDEC_TYPE_AUDIO_DECODER_ADPCM, NULL);
    default:
      return NULL;
  }
}

static gboolean
swfdec_audio_decoder_builtin_prepare (guint codec, SwfdecAudioFormat format, char **detail)
{
  return codec == SWFDEC_AUDIO_CODEC_UNCOMPRESSED ||
    codec == SWFDEC_AUDIO_CODEC_UNDEFINED ||
    codec == SWFDEC_AUDIO_CODEC_ADPCM;
}

static const struct {
  const char *		name;
  SwfdecAudioDecoder *	(* func) (guint, SwfdecAudioFormat);
  gboolean		(* prepare) (guint, SwfdecAudioFormat, char **);
} audio_codecs[] = {
  { "builtin",	swfdec_audio_decoder_builtin_new, swfdec_audio_decoder_builtin_prepare },
#ifdef HAVE_GST
  { "gst",	swfdec_audio_decoder_gst_new, swfdec_audio_decoder_gst_prepare },
#endif
};

gboolean
swfdec_audio_decoder_prepare (guint codec, SwfdecAudioFormat format, char **missing)
{
  char *detail = NULL, *s = NULL;
  guint i;
  
  for (i = 0; i < G_N_ELEMENTS (audio_codecs); i++) {
    if (audio_codecs[i].prepare (codec, format, &s)) {
      g_free (detail);
      g_free (s);
      if (missing)
	*missing = NULL;
      return TRUE;
    }
    if (s) {
      if (detail == NULL)
	detail = s;
      else
	g_free (s);
      s = NULL;
    }
  }
  if (missing)
    *missing = detail;
  return FALSE;
}

/**
 * swfdec_audio_decoder_new:
 * @format: #SwfdecAudioCodec to decode
 *
 * Creates a decoder suitable for decoding @format. If no decoder is available
 * for the given for mat, %NULL is returned.
 *
 * Returns: a new decoder or %NULL
 **/
SwfdecAudioDecoder *
swfdec_audio_decoder_new (guint codec, SwfdecAudioFormat format)
{
  SwfdecAudioDecoder *ret;
  guint i;

  g_return_val_if_fail (SWFDEC_IS_AUDIO_FORMAT (format), NULL);

  for (i = 0; i < G_N_ELEMENTS (audio_codecs); i++) {
    ret = audio_codecs[i].func (codec, format);
    if (ret)
      break;
  }

  if (ret == NULL) {
    ret = g_object_new (SWFDEC_TYPE_AUDIO_DECODER, NULL);
    swfdec_audio_decoder_error (ret, "no suitable decoder for audio codec %u", codec);
  }

  ret->codec = codec;
  ret->format = format;

  return ret;
}

/**
 * swfdec_audio_decoder_push:
 * @decoder: a #SwfdecAudioDecoder
 * @buffer: a #SwfdecBuffer to process or %NULL to flush
 *
 * Pushes a new buffer into the decoding pipeline. After this the results can
 * be queried using swfdec_audio_decoder_pull(). Some decoders may not decode
 * all available data immediately. So when you are done decoding, you may want
 * to flush the decoder. Flushing can be achieved by passing %NULL as the 
 * @buffer argument. Do this when you are finished decoding.
 **/
void
swfdec_audio_decoder_push (SwfdecAudioDecoder *decoder, SwfdecBuffer *buffer)
{
  SwfdecAudioDecoderClass *klass;

  g_return_if_fail (SWFDEC_IS_AUDIO_DECODER (decoder));

  if (decoder->error)
    return;
  klass = SWFDEC_AUDIO_DECODER_GET_CLASS (decoder);
  klass->push (decoder, buffer);
}

/**
 * swfdec_audio_decoder_pull:
 * @decoder: a #SwfdecAudioDecoder
 *
 * Gets the next buffer of decoded audio data. Since some decoders do not
 * produce one output buffer per input buffer, any number of buffers may be
 * available after calling swfdec_audio_decoder_push(), even none. When no more
 * buffers are available, this function returns %NULL. You need to provide more
 * input in then. A simple decoding pipeline would look like this:
 * <informalexample><programlisting>do {
 *   input = next_input_buffer ();
 *   swfdec_audio_decoder_push (decoder, input);
 *   while ((output = swfdec_audio_decoder_pull (decoder))) {
 *     ... process output ...
 *   }
 * } while (input != NULL); </programlisting></informalexample>
 *
 * Returns: the next buffer or %NULL if no more buffers are available.
 **/
SwfdecBuffer *
swfdec_audio_decoder_pull (SwfdecAudioDecoder *decoder)
{
  SwfdecAudioDecoderClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AUDIO_DECODER (decoder), NULL);

  if (decoder->error)
    return NULL;
  klass = SWFDEC_AUDIO_DECODER_GET_CLASS (decoder);
  return klass->pull (decoder);
}

void
swfdec_audio_decoder_error (SwfdecAudioDecoder *decoder, const char *error, ...)
{
  va_list args;

  g_return_if_fail (SWFDEC_IS_AUDIO_DECODER (decoder));
  g_return_if_fail (error != NULL);

  va_start (args, error);
  swfdec_audio_decoder_errorv (decoder, error, args);
  va_end (args);
}

void
swfdec_audio_decoder_errorv (SwfdecAudioDecoder *decoder, const char *error, va_list args)
{
  char *real;

  g_return_if_fail (SWFDEC_IS_AUDIO_DECODER (decoder));
  g_return_if_fail (error != NULL);

  real = g_strdup_vprintf (error, args);
  SWFDEC_ERROR ("error decoding audio: %s", real);
  g_free (real);
  decoder->error = TRUE;
}

