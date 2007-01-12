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
#include "swfdec_flv_decoder.h"
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
swfdec_decoder_new (SwfdecPlayer *player, SwfdecBufferQueue *queue)
{
  guchar *data;
  SwfdecBuffer *buffer;
  SwfdecDecoder *retval;
  
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (queue != NULL, NULL);

  if (!swfdec_decoder_can_detect (queue))
    return NULL;

  buffer = swfdec_buffer_queue_peek (queue, 3);
  data = buffer->data;
  if ((data[0] == 'C' || data[0] == 'F') &&
      data[1] == 'W' &&
      data[2] == 'S') {
    retval = g_object_new (SWFDEC_TYPE_SWF_DECODER, NULL);
  } else if (data[0] == 'F' &&
      data[1] == 'L' &&
      data[2] == 'V') {
    retval = g_object_new (SWFDEC_TYPE_FLV_DECODER, NULL);
  } else {
    retval = NULL;
  }
  swfdec_buffer_unref (buffer);
  if (retval) {
    retval->player = player;
    retval->queue = queue;
  }
  return retval;
}

