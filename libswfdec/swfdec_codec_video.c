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

#include "swfdec_codec_video.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

/**
 * swfdec_video_decoder_new:
 * @format: #SwfdecVideoFormat to create the #SwfdecVideoDecoder for
 *
 * Creates a new decoder to decode videos of type @format. If no suitable
 * decoder could be created, %NULL is returned.
 *
 * Returns:
 **/
SwfdecVideoDecoder *
swfdec_video_decoder_new (SwfdecVideoFormat format)
{
  SwfdecVideoDecoder *ret;

  ret = swfdec_video_decoder_screen_new (format);
#ifdef HAVE_FFMPEG
  if (ret == NULL)
    ret = swfdec_video_decoder_ffmpeg_new (format);
#endif
#ifdef HAVE_GST
  if (ret == NULL)
    ret = swfdec_video_decoder_gst_new (format);
#endif

  if (ret != NULL) {
    ret->format = format;
    g_return_val_if_fail (ret->decode, ret);
    g_return_val_if_fail (ret->free, ret);
  } else {
    SWFDEC_WARNING ("no decoder found for codec %u", (guint) format);
  }
  return ret;
}

/**
 * swfdec_video_decoder_free:
 * @decoder: a #SwfdecVideoDecoder
 *
 * Frees the given @decoder and all associated ressources.
 **/
void
swfdec_video_decoder_free (SwfdecVideoDecoder *decoder)
{
  g_return_if_fail (decoder);

  decoder->free (decoder);
}

/**
 * swfdec_video_decoder_decode:
 * @decoder: a #SwfdecVideoDecoder
 * @buffer: buffer to decode
 *
 * Decodes the given buffer into an image surface.
 *
 * Returns: a new cairo image surface or %NULL on error.
 **/
cairo_surface_t *
swfdec_video_decoder_decode (SwfdecVideoDecoder *decoder, SwfdecBuffer *buffer)
{
  static const cairo_user_data_key_t key;
  cairo_surface_t *surface;
  guint width, height, rowstride;

  g_return_val_if_fail (decoder != NULL, NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  if (decoder->format == SWFDEC_VIDEO_FORMAT_VP6) {
    guint wsub, hsub;
    SwfdecBuffer *tmp;
    wsub = *buffer->data;
    hsub = wsub & 0xF;
    wsub >>= 4;
    tmp = swfdec_buffer_new_subbuffer (buffer, 1, buffer->length - 1);
    buffer = decoder->decode (decoder, tmp, &width, &height, &rowstride);
    swfdec_buffer_unref (tmp);
    if (wsub >= width || hsub >= height) {
      SWFDEC_ERROR ("size subtraction too big");
      if (buffer)
	swfdec_buffer_unref (buffer);
      return NULL;
    }
    width -= wsub;
    height -= hsub;
  } else {
    buffer = decoder->decode (decoder, buffer, &width, &height, &rowstride);
  }
  if (buffer == NULL) {
    SWFDEC_ERROR ("failed to decode video");
    return NULL;
  }
  if (width == 0 || height == 0 || rowstride < width * 4) {
    SWFDEC_ERROR ("invalid image size");
    swfdec_buffer_unref (buffer);
    return NULL;
  }
  surface = cairo_image_surface_create_for_data (buffer->data, CAIRO_FORMAT_RGB24,
      width, height, rowstride);
  if (cairo_surface_status (surface)) {
    SWFDEC_ERROR ("failed to create surface: %s", 
	cairo_status_to_string (cairo_surface_status (surface)));
    cairo_surface_destroy (surface);
    swfdec_buffer_unref (buffer);
    return NULL;
  }
  cairo_surface_set_user_data (surface, &key, buffer, 
      (cairo_destroy_func_t) swfdec_buffer_unref);
  return surface;
}

