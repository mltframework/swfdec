/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_image_decoder.h"
#include "swfdec_debug.h"
#include "swfdec_image.h"

G_DEFINE_TYPE (SwfdecImageDecoder, swfdec_image_decoder, SWFDEC_TYPE_DECODER)

static void
swfdec_image_decoder_dispose (GObject *object)
{
  SwfdecImageDecoder *image = SWFDEC_IMAGE_DECODER (object);

  if (image->queue) {
    swfdec_buffer_queue_unref (image->queue);
    image->queue = NULL;
  }

  if (image->image) {
    g_object_unref (image->image);
    image->image = NULL;
  }

  G_OBJECT_CLASS (swfdec_image_decoder_parent_class)->dispose (object);
}

static SwfdecStatus
swfdec_image_decoder_parse (SwfdecDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecImageDecoder *image = SWFDEC_IMAGE_DECODER (dec);

  if (image->queue == NULL)
    image->queue = swfdec_buffer_queue_new ();
  swfdec_buffer_queue_push (image->queue, buffer);
  dec->bytes_loaded += buffer->length;
  if (dec->bytes_loaded > dec->bytes_total)
    dec->bytes_total = dec->bytes_loaded;
  return 0;
}

/* FIXME: move this to swfdec_image API? */
static gboolean
swfdec_image_get_size (SwfdecImage *image, guint *w, guint *h)
{
  cairo_surface_t *surface;

  surface = swfdec_image_create_surface (image, NULL);
  if (surface == NULL)
    return FALSE;

  if (w)
    *w = cairo_image_surface_get_width (surface);
  if (h)
    *h = cairo_image_surface_get_width (surface);

  cairo_surface_destroy (surface);

  return TRUE;
}

static SwfdecStatus
swfdec_image_decoder_eof (SwfdecDecoder *dec)
{
  SwfdecImageDecoder *image = SWFDEC_IMAGE_DECODER (dec);
  SwfdecBuffer *buffer;
  guint depth;

  if (image->queue == NULL)
    return 0;
  depth = swfdec_buffer_queue_get_depth (image->queue);
  if (depth == 0) {
    swfdec_buffer_queue_unref (image->queue);
    image->queue = NULL;
    return SWFDEC_STATUS_ERROR;
  }
  buffer = swfdec_buffer_queue_pull (image->queue,
      swfdec_buffer_queue_get_depth (image->queue));
  swfdec_buffer_queue_unref (image->queue);
  image->queue = NULL;
  image->image = swfdec_image_new (buffer);
  if (!swfdec_image_get_size (image->image, &dec->width, &dec->height))
    return SWFDEC_STATUS_ERROR;
  dec->frames_loaded = 1;
  dec->frames_total = 1;
  if (image->image->type == SWFDEC_IMAGE_TYPE_JPEG)
    dec->data_type = SWFDEC_LOADER_DATA_JPEG;
  else if (image->image->type == SWFDEC_IMAGE_TYPE_PNG)
    dec->data_type = SWFDEC_LOADER_DATA_PNG;


  return SWFDEC_STATUS_INIT | SWFDEC_STATUS_IMAGE;
}

static void
swfdec_image_decoder_class_init (SwfdecImageDecoderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  SwfdecDecoderClass *decoder_class = SWFDEC_DECODER_CLASS (class);

  object_class->dispose = swfdec_image_decoder_dispose;

  decoder_class->parse = swfdec_image_decoder_parse;
  decoder_class->eof = swfdec_image_decoder_eof;
}

static void
swfdec_image_decoder_init (SwfdecImageDecoder *s)
{
}

