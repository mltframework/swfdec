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

#include <liboil/liboil.h>
#include "swfdec_video_decoder.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_renderer_internal.h"

G_DEFINE_TYPE (SwfdecVideoDecoder, swfdec_video_decoder, G_TYPE_OBJECT)

static void
swfdec_video_decoder_class_init (SwfdecVideoDecoderClass *klass)
{
}

static void
swfdec_video_decoder_init (SwfdecVideoDecoder *video_decoder)
{
}

static GSList *video_codecs = NULL;

void
swfdec_video_decoder_register (GType type)
{
  g_return_if_fail (g_type_is_a (type, SWFDEC_TYPE_VIDEO_DECODER));

  video_codecs = g_slist_append (video_codecs, GSIZE_TO_POINTER ((gsize) type));
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
    case SWFDEC_VIDEO_CODEC_H264:
      return SWFDEC_VIDEO_FORMAT_I420;
    case SWFDEC_VIDEO_CODEC_UNDEFINED:
    case SWFDEC_VIDEO_CODEC_SCREEN:
    case SWFDEC_VIDEO_CODEC_SCREEN2:
      return SWFDEC_VIDEO_FORMAT_RGBA;
    default:
      SWFDEC_ERROR ("unknown codec %u, assuming RGBA format", codec);
      return SWFDEC_VIDEO_FORMAT_RGBA;
  }
}

gboolean
swfdec_video_decoder_prepare (guint codec, char **missing)
{
  char *detail = NULL, *s = NULL;
  GSList *walk;
  
  for (walk = video_codecs; walk; walk = walk->next) {
    SwfdecVideoDecoderClass *klass = g_type_class_ref (GPOINTER_TO_SIZE (walk->data));
    if (klass->prepare (codec, &s)) {
      g_free (detail);
      g_free (s);
      if (missing)
	*missing = NULL;
      g_type_class_unref (klass);
      return TRUE;
    }
    if (s) {
      if (detail == NULL)
	detail = s;
      else
	g_free (s);
      s = NULL;
    }
    g_type_class_unref (klass);
  }
  if (missing)
    *missing = detail;
  else
    g_free (detail);
  return FALSE;
}

/**
 * swfdec_video_decoder_new:
 * @codec: codec id
 * @data: initialization data for the video codec or %NULL if none. Currently 
 *        only used for H264
 *
 * Creates a decoder suitable for decoding @format. If no decoder is available
 * for the given for mat, %NULL is returned.
 *
 * Returns: a new decoder or %NULL
 **/
SwfdecVideoDecoder *
swfdec_video_decoder_new (guint codec, SwfdecBuffer *buffer)
{
  SwfdecVideoDecoder *ret = NULL;
  GSList *walk;
  
  for (walk = video_codecs; walk; walk = walk->next) {
    SwfdecVideoDecoderClass *klass = g_type_class_ref (GPOINTER_TO_SIZE (walk->data));
    ret = klass->create (codec, buffer);
    g_type_class_unref (klass);
    if (ret)
      break;
  }

  if (ret == NULL) {
    ret = g_object_new (SWFDEC_TYPE_VIDEO_DECODER, NULL);
    swfdec_video_decoder_error (ret, "no suitable decoder for video codec %u", codec);
  }

  ret->codec = codec;

  return ret;
}

/**
 * swfdec_video_decoder_decode:
 * @decoder: a #SwfdecVideoDecoder
 * @buffer: a #SwfdecBuffer to process
 *
 * Hands the decoder a new buffer for decoding. The buffer should be decoded 
 * immediately. It is assumed that width, height and data areas are set to the 
 * correct values upon return from this function.
 **/
void
swfdec_video_decoder_decode (SwfdecVideoDecoder *decoder, SwfdecBuffer *buffer)
{
  SwfdecVideoDecoderClass *klass;

  g_return_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder));

  if (decoder->error)
    return;
  klass = SWFDEC_VIDEO_DECODER_GET_CLASS (decoder);
  klass->decode (decoder, buffer);
}

guint
swfdec_video_decoder_get_codec (SwfdecVideoDecoder *decoder)
{
  g_return_val_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder), SWFDEC_VIDEO_CODEC_UNDEFINED);

  return decoder->codec;
}

gboolean
swfdec_video_decoder_get_error (SwfdecVideoDecoder *decoder)
{
  g_return_val_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder), TRUE);

  return decoder->error;
}

guint
swfdec_video_decoder_get_width (SwfdecVideoDecoder *decoder)
{
  g_return_val_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder), 0);

  return decoder->width;
}

guint
swfdec_video_decoder_get_height (SwfdecVideoDecoder *decoder)
{
  g_return_val_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder), 0);

  return decoder->height;
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
swfdec_video_i420_to_rgb (SwfdecVideoDecoder *decoder)
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

  halfwidth = (decoder->width + 1)>>1;
  tmp = g_malloc (4 * decoder->width * decoder->height);
  tmp_u = g_malloc (decoder->width);
  tmp_v = g_malloc (decoder->width);
  tmp1 = g_malloc (halfwidth);
  argb_image = g_malloc (4 * decoder->width * decoder->height);

  yp = decoder->plane[0];
  up = decoder->plane[1];
  vp = decoder->plane[2];
  argbp = argb_image;
  halfheight = (decoder->height+1)>>1;
  for(j=0;(guint)j<decoder->height;j++){
    guint32 weight = 192 - 128*(j&1);

    oil_merge_linear_u8(tmp1,
        up + decoder->rowstride[1] * CLAMP((j-1)/2,0,halfheight-1),
        up + decoder->rowstride[1] * CLAMP((j+1)/2,0,halfheight-1),
        &weight, halfwidth);
    upsample (tmp_u, tmp1, decoder->width);
    oil_merge_linear_u8(tmp1,
        vp + decoder->rowstride[2] * CLAMP((j-1)/2,0,halfheight-1),
        vp + decoder->rowstride[2] * CLAMP((j+1)/2,0,halfheight-1),
        &weight, halfwidth);
    upsample (tmp_v, tmp1, decoder->width);

    yuv_mux (tmp, yp, tmp_u, tmp_v, decoder->width);
    oil_colorspace_argb(argbp, tmp, jfif_matrix, decoder->width);
    yp += decoder->rowstride[0];
    argbp += decoder->width;
  }
  g_free(tmp);
  g_free(tmp_u);
  g_free(tmp_v);
  g_free(tmp1);
  return (unsigned char *)argb_image;
}

/* FIXME: use liboil (or better: cairo) for this */
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

cairo_surface_t *
swfdec_video_decoder_get_image (SwfdecVideoDecoder *decoder, SwfdecRenderer *renderer)
{
  cairo_surface_t *surface;
  guint rowstride;
  guint8 *data;

  g_return_val_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder), NULL);
  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);

  if (decoder->error)
    return NULL;
  /* special case: If decoding an image failed, setting plane[0] to NULL means
   * "currently no image available". This is generally only useful for the 
   * first image, as otherwise the video decoder is meant to keep the last image. */
  if (decoder->plane[0] == NULL)
    return NULL;

  if (swfdec_video_codec_get_format (decoder->codec) == SWFDEC_VIDEO_FORMAT_I420) {
    data = swfdec_video_i420_to_rgb (decoder);
    if (data == NULL) {
      SWFDEC_ERROR ("I420 => RGB conversion failed");
      return NULL;
    }
    rowstride = decoder->width * 4;
  } else {
    rowstride = decoder->rowstride[0];
    data = g_memdup (decoder->plane[0], rowstride * decoder->height);
  }
  if (decoder->mask) {
    swfdec_video_codec_apply_mask (data, rowstride, decoder->mask, 
	decoder->mask_rowstride, decoder->width, decoder->height);
  }
  surface = swfdec_renderer_create_for_data (renderer, data,
      decoder->mask ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
      decoder->width, decoder->height, rowstride);
  return surface;
}

void
swfdec_video_decoder_error (SwfdecVideoDecoder *decoder, const char *error, ...)
{
  va_list args;

  g_return_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder));
  g_return_if_fail (error != NULL);

  va_start (args, error);
  swfdec_video_decoder_errorv (decoder, error, args);
  va_end (args);
}

void
swfdec_video_decoder_errorv (SwfdecVideoDecoder *decoder, const char *error, va_list args)
{
  char *real;

  g_return_if_fail (SWFDEC_IS_VIDEO_DECODER (decoder));
  g_return_if_fail (error != NULL);

  real = g_strdup_vprintf (error, args);
  SWFDEC_ERROR ("error decoding video: %s", real);
  g_free (real);
  decoder->error = TRUE;
}

