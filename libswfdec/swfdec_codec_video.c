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
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

/**
 * swfdec_video_decoder_new:
 * @codec: #SwfdecVideoCodec to create the #SwfdecVideoDecoder for
 *
 * Creates a new decoder to decode videos of type @codec. If no suitable
 * decoder could be created, %NULL is returned.
 *
 * Returns:
 **/
SwfdecVideoDecoder *
swfdec_video_decoder_new (SwfdecVideoCodec codec)
{
  SwfdecVideoDecoder *ret;

  ret = swfdec_video_decoder_screen_new (codec);
  if (ret == NULL)
    ret = swfdec_video_decoder_vp6_alpha_new (codec);
#ifdef HAVE_FFMPEG
  if (ret == NULL)
    ret = swfdec_video_decoder_ffmpeg_new (codec);
#endif
#ifdef HAVE_GST
  if (ret == NULL)
    ret = swfdec_video_decoder_gst_new (codec);
#endif

  if (ret != NULL) {
    ret->codec = codec;
    g_return_val_if_fail (ret->decode, ret);
    g_return_val_if_fail (ret->free, ret);
  } else {
    SWFDEC_WARNING ("no decoder found for codec %u", (guint) codec);
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

#ifdef HAVE_FFMPEG
#define swfdec_video_i420_to_rgb swfdec_video_ffmpeg_i420_to_rgb
#else
static guint8 *
swfdec_video_i420_to_rgb (&image)
{
  SWFDEC_FIXME ("implement I420 => RGB without ffmpeg");
  return NULL;
}
#endif /* HAVE_FFMPEG */

/* FIXME: use liboil for this */
static void
swfdec_video_codec_apply_mask (guint8 *data, guint rowstride, const guint8 *mask,
    guint mask_rowstride, guint width, guint height)
{
  const guint8 *in;
  guint8 *out;
  guint x, y;

  data += SWFDEC_COLOR_INDEX_ALPHA;
  for (y = 0; y < height; y++) {
    in = mask;
    out = data;
    for (x = 0; x < width; x++) {
      *out = *in;
      out += 4;
      in++;
    }
    mask += mask_rowstride;
    data += rowstride;
  }
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
  SwfdecVideoImage image;
  static const cairo_user_data_key_t key;
  cairo_surface_t *surface;
  guint8 *data;

  g_return_val_if_fail (decoder != NULL, NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  if (!decoder->decode (decoder, buffer, &image)) {
    SWFDEC_ERROR ("failed to decode video");
    return NULL;
  }
  g_assert (image.width != 0 && image.height != 0);
  /* FIXME: use cairo for all of this when cairo accelerates it */
  if (swfdec_video_codec_get_format (decoder->codec) == SWFDEC_VIDEO_FORMAT_I420) {
    data = swfdec_video_i420_to_rgb (&image);
    if (data == NULL) {
      SWFDEC_ERROR ("I420 => RGB conversion failed");
      return NULL;
    }
  } else {
    data = g_memdup (image.plane[0], image.rowstride[0] * image.height);
  }
  if (image.mask) {
    swfdec_video_codec_apply_mask (data, image.width * 4, image.mask, 
	image.mask_rowstride, image.width, image.height);
  }
  surface = cairo_image_surface_create_for_data (data, 
      image.mask ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
      image.width, image.height, image.width * 4);
  if (cairo_surface_status (surface)) {
    SWFDEC_ERROR ("failed to create surface: %s", 
	cairo_status_to_string (cairo_surface_status (surface)));
    cairo_surface_destroy (surface);
    return NULL;
  }
  cairo_surface_set_user_data (surface, &key, data, 
      (cairo_destroy_func_t) g_free);
  return surface;
}

/**
 * swfdec_video_codec_get_format:
 * @codec: codec to check
 *
 * Returns the output format used for this codec. Video codecs must use these
 * codecs when decoding video.
 *
 * Returns: the output format to use for this format
 **/
SwfdecVideoFormat
swfdec_video_codec_get_format (SwfdecVideoCodec codec)
{
  switch (codec) {
    case SWFDEC_VIDEO_CODEC_H263:
    case SWFDEC_VIDEO_CODEC_VP6:
    case SWFDEC_VIDEO_CODEC_VP6_ALPHA:
      return SWFDEC_VIDEO_FORMAT_I420;
    default:
      return SWFDEC_VIDEO_FORMAT_RGBA;
  }
}

