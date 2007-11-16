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
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_image.h"
#include "swfdec_image_decoder.h"
#include "swfdec_swf_decoder.h"


G_DEFINE_ABSTRACT_TYPE (SwfdecDecoder, swfdec_decoder, G_TYPE_OBJECT)

static void
swfdec_decoder_class_init (SwfdecDecoderClass *klass)
{
}

static void
swfdec_decoder_init (SwfdecDecoder *decoder)
{
}

SwfdecDecoder *
swfdec_decoder_new (SwfdecPlayer *player, const SwfdecBuffer *buffer)
{
  guchar *data;
  SwfdecDecoder *retval;
  
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  if (buffer->length < SWFDEC_DECODER_DETECT_LENGTH)
    return NULL;

  data = buffer->data;
  if ((data[0] == 'C' || data[0] == 'F') &&
      data[1] == 'W' &&
      data[2] == 'S') {
    retval = g_object_new (SWFDEC_TYPE_SWF_DECODER, NULL);
#if 0
  } else if (data[0] == 'F' &&
      data[1] == 'L' &&
      data[2] == 'V') {
    retval = g_object_new (SWFDEC_TYPE_FLV_DECODER, NULL);
#endif
  } else if (swfdec_image_detect (data) != SWFDEC_IMAGE_TYPE_UNKNOWN) {
    retval = g_object_new (SWFDEC_TYPE_IMAGE_DECODER, NULL);
  } else {
    retval = NULL;
  }
  if (retval) {
    retval->player = player;
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

