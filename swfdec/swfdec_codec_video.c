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

#include <liboil/liboil.h>
#include "swfdec_codec_video.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

static gboolean
swfdec_video_decoder_builtin_prepare (guint codec, char **detail)
{
  return codec == SWFDEC_VIDEO_CODEC_SCREEN;
}

static SwfdecVideoDecoder *
swfdec_video_decoder_builtin_new (guint codec)
{
  SwfdecVideoDecoder *ret;

  ret = swfdec_video_decoder_screen_new (codec);
  if (ret == NULL)
    ret = swfdec_video_decoder_vp6_alpha_new (codec);

  return ret;
}

static const struct {
  const char *		name;
  SwfdecVideoDecoder *	(* func) (guint);
  gboolean		(* prepare) (guint, char **);
} video_codecs[] = {
    { "builtin",	swfdec_video_decoder_builtin_new,	swfdec_video_decoder_builtin_prepare }
#ifdef HAVE_GST
  , { "gst",		swfdec_video_decoder_gst_new,		swfdec_video_decoder_gst_prepare }
#endif
#ifdef HAVE_FFMPEG
  , { "ffmpeg",		swfdec_video_decoder_ffmpeg_new,	swfdec_video_decoder_ffmpeg_prepare }
#endif
};

char *
swfdec_video_decoder_prepare (guint codec)
{
  char *detail = NULL, *s = NULL;
  guint i;
  
  /* the builtin codec is implemented as a wrapper around VP6 */
  if (codec == SWFDEC_VIDEO_CODEC_VP6_ALPHA)
    codec = SWFDEC_VIDEO_CODEC_VP6;

  for (i = 0; i < G_N_ELEMENTS (video_codecs); i++) {
    if (video_codecs[i].prepare (codec, &s)) {
      g_free (detail);
      g_free (s);
      return NULL;
    }
    if (s) {
      if (detail == NULL)
	detail = s;
      else
	g_free (s);
      s = NULL;
    }
  }
  return detail;
}

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
swfdec_video_decoder_new (guint codec)
{
  SwfdecVideoDecoder *ret = NULL;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (video_codecs); i++) {
    ret = video_codecs[i].func (codec);
    if (ret)
      break;
  }

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

#define oil_argb(a,r,g,b) (((a) << 24) | ((r) << 16) | ((g) << 8) | b)
static gint16 jfif_matrix[24] = {
  0,      0,      -8192,   -8192,
  16384,  0,      0,       0,
  0,      16384,  16384,   16384,
  0,      0,      -5638,   29032,
  0,      22970,  -11700,  0,
  0, 0, 0, 0
};

static void
yuv_mux (guint32 *dest, const guint8 *src_y, const guint8 *src_u, const guint8 *src_v,
    int n)
{
  int i;
  for (i = 0; i < n; i++) {
    dest[i] = oil_argb(255, src_y[i], src_u[i], src_v[i]);
  }
}

static void
upsample (guint8 *d, guint8 *s, int n)
{
  int i;

  d[0] = s[0];

  for (i = 0; i < n-3; i+=2) {
    d[i + 1] = (3*s[i/2] + s[i/2+1] + 2)>>2;
    d[i + 2] = (s[i/2] + 3*s[i/2+1] + 2)>>2;
  }

  if (n&1) {
    i = n-3;
    d[n-2] = s[n/2];
    d[n-1] = s[n/2];
  } else {
    d[n-1] = s[n/2-1];
  }
}

static guint8 *
swfdec_video_i420_to_rgb (SwfdecVideoImage *image)
{
  guint32 *tmp;
  guint8 *tmp_u;
  guint8 *tmp_v;
  guint8 *tmp1;
  guint32 *argb_image;
  const guint8 *yp, *up, *vp;
  guint32 *argbp;
  int j;
  guint halfwidth;
  int halfheight;

  halfwidth = (image->width + 1)>>1;
  tmp = g_malloc (4 * image->width * image->height);
  tmp_u = g_malloc (image->width);
  tmp_v = g_malloc (image->width);
  tmp1 = g_malloc (halfwidth);
  argb_image = g_malloc (4 * image->width * image->height);

  yp = image->plane[0];
  up = image->plane[1];
  vp = image->plane[2];
  argbp = argb_image;
  halfheight = (image->height+1)>>1;
  for(j=0;(guint)j<image->height;j++){
    guint32 weight = 192 - 128*(j&1);

    oil_merge_linear_u8(tmp1,
        up + image->rowstride[1] * CLAMP((j-1)/2,0,halfheight-1),
        up + image->rowstride[1] * CLAMP((j+1)/2,0,halfheight-1),
        &weight, halfwidth);
    upsample (tmp_u, tmp1, image->width);
    oil_merge_linear_u8(tmp1,
        vp + image->rowstride[2] * CLAMP((j-1)/2,0,halfheight-1),
        vp + image->rowstride[2] * CLAMP((j+1)/2,0,halfheight-1),
        &weight, halfwidth);
    upsample (tmp_v, tmp1, image->width);

    yuv_mux (tmp, yp, tmp_u, tmp_v, image->width);
    oil_colorspace_argb(argbp, tmp, jfif_matrix, image->width);
    yp += image->rowstride[0];
    argbp += image->width;
  }
  g_free(tmp);
  g_free(tmp_u);
  g_free(tmp_v);
  g_free(tmp1);
  return (unsigned char *)argb_image;
}

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
  guint rowstride;

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
    rowstride = image.width * 4;
  } else {
    rowstride = image.rowstride[0];
    data = g_memdup (image.plane[0], rowstride * image.height);
  }
  if (image.mask) {
    swfdec_video_codec_apply_mask (data, image.width * 4, image.mask, 
	image.mask_rowstride, image.width, image.height);
  }
  surface = cairo_image_surface_create_for_data (data, 
      image.mask ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
      image.width, image.height, rowstride);
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
swfdec_video_codec_get_format (guint codec)
{
  switch (codec) {
    case SWFDEC_VIDEO_CODEC_H263:
    case SWFDEC_VIDEO_CODEC_VP6:
    case SWFDEC_VIDEO_CODEC_VP6_ALPHA:
      return SWFDEC_VIDEO_FORMAT_I420;
    case SWFDEC_VIDEO_CODEC_UNDEFINED:
    case SWFDEC_VIDEO_CODEC_SCREEN:
    case SWFDEC_VIDEO_CODEC_SCREEN2:
      return SWFDEC_VIDEO_FORMAT_RGBA;
    default:
      g_return_val_if_reached (SWFDEC_VIDEO_FORMAT_RGBA);
  }
}

