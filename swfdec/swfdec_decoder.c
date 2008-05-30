/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_decoder.h"
#include "swfdec_audio_decoder.h"
#include "swfdec_codec_video.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_image.h"
#include "swfdec_image_decoder.h"
#include "swfdec_swf_decoder.h"

enum {
  MISSING_PLUGINS,
  LAST_SIGNAL
};

G_DEFINE_ABSTRACT_TYPE (SwfdecDecoder, swfdec_decoder, G_TYPE_OBJECT)
static guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_decoder_class_init (SwfdecDecoderClass *klass)
{
  /**
   * SwfdecDecoder::missing-plugin:
   * @player: the #SwfdecPlayer missing plugins
   * @details: the detail string for the missing plugins
   *
   * Emitted whenever a missing plugin is detected.
   */
  signals[MISSING_PLUGINS] = g_signal_new ("missing-plugin", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SwfdecDecoderClass, missing_plugins),
      NULL, NULL, g_cclosure_marshal_VOID__STRING,
      G_TYPE_NONE, 1, G_TYPE_STRING);

}

static void
swfdec_decoder_init (SwfdecDecoder *decoder)
{
}

SwfdecDecoder *
swfdec_decoder_new (const SwfdecBuffer *buffer)
{
  guchar *data;
  SwfdecDecoder *retval;
  
  g_return_val_if_fail (buffer != NULL, NULL);

  if (buffer->length < SWFDEC_DECODER_DETECT_LENGTH)
    return NULL;

  data = buffer->data;
  if ((data[0] == 'C' || data[0] == 'F') &&
      data[1] == 'W' &&
      data[2] == 'S') {
    retval = g_object_new (SWFDEC_TYPE_SWF_DECODER, NULL);
  } else if (swfdec_image_detect (data) != SWFDEC_IMAGE_TYPE_UNKNOWN) {
    retval = g_object_new (SWFDEC_TYPE_IMAGE_DECODER, NULL);
  } else {
    retval = NULL;
  }
  return retval;
}

SwfdecStatus
swfdec_decoder_parse (SwfdecDecoder *decoder, SwfdecBuffer *buffer)
{
  SwfdecDecoderClass *klass;

  g_return_val_if_fail (SWFDEC_IS_DECODER (decoder), SWFDEC_STATUS_ERROR);
  g_return_val_if_fail (buffer != NULL, SWFDEC_STATUS_ERROR);

  klass = SWFDEC_DECODER_GET_CLASS (decoder);
  g_return_val_if_fail (klass->parse, SWFDEC_STATUS_ERROR);
  return klass->parse (decoder, buffer);
}

SwfdecStatus
swfdec_decoder_eof (SwfdecDecoder *decoder)
{
  SwfdecDecoderClass *klass;

  g_return_val_if_fail (SWFDEC_IS_DECODER (decoder), SWFDEC_STATUS_ERROR);

  klass = SWFDEC_DECODER_GET_CLASS (decoder);
  g_return_val_if_fail (klass->eof, SWFDEC_STATUS_ERROR);
  return klass->eof (decoder);
}

void
swfdec_decoder_use_audio_codec (SwfdecDecoder *decoder, guint codec, 
    SwfdecAudioFormat format)
{
  char *detail;

  g_return_if_fail (SWFDEC_IS_DECODER (decoder));

  swfdec_audio_decoder_prepare (codec, format, &detail);
  if (detail == NULL)
    return;

  SWFDEC_INFO ("missing audio plugin: %s\n", detail);
  g_signal_emit (decoder, signals[MISSING_PLUGINS], 0, detail);
  g_free (detail);
}

void
swfdec_decoder_use_video_codec (SwfdecDecoder *decoder, guint codec)
{
  char *detail;

  g_return_if_fail (SWFDEC_IS_DECODER (decoder));

  detail = swfdec_video_decoder_prepare (codec);
  if (detail == NULL)
    return;

  SWFDEC_INFO ("missing video plugin: %s\n", detail);
  g_signal_emit (decoder, signals[MISSING_PLUGINS], 0, detail);
  g_free (detail);
}

