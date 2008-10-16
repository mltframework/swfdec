/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_video_decoder_vp6_alpha.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecVideoDecoderVp6Alpha, swfdec_video_decoder_vp6_alpha, SWFDEC_TYPE_VIDEO_DECODER)

static gboolean
swfdec_video_decoder_vp6_alpha_prepare (guint codec, char **missing)
{
  if (codec != SWFDEC_VIDEO_CODEC_VP6_ALPHA)
    return FALSE;

  return swfdec_video_decoder_prepare (SWFDEC_VIDEO_CODEC_VP6, missing);
}

static SwfdecVideoDecoder *
swfdec_video_decoder_vp6_alpha_create (guint codec, SwfdecBuffer *buffer)
{
  if (codec != SWFDEC_VIDEO_CODEC_VP6_ALPHA)
    return NULL;

  return g_object_new (SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA, NULL);
}

static void
swfdec_video_decoder_vp6_alpha_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecVideoDecoderVp6Alpha *vp6 = SWFDEC_VIDEO_DECODER_VP6_ALPHA (dec);
  SwfdecBuffer *tmp;
  SwfdecBits bits;
  guint size;

  swfdec_bits_init (&bits, buffer);
  size = swfdec_bits_get_bu24 (&bits);
  tmp = swfdec_bits_get_buffer (&bits, size);
  if (tmp == NULL) {
    swfdec_video_decoder_error (dec, "invalid buffer size for image buffer");
    return;
  }
  swfdec_video_decoder_decode (vp6->image, tmp);
  swfdec_buffer_unref (tmp);
  tmp = swfdec_bits_get_buffer (&bits, -1);
  if (tmp == NULL) {
    swfdec_video_decoder_error (dec, "invalid buffer size for mask buffer");
    return;
  }
  swfdec_video_decoder_decode (vp6->mask, tmp);
  swfdec_buffer_unref (tmp);
  if (swfdec_video_decoder_get_error (vp6->image) ||
      swfdec_video_decoder_get_error (vp6->mask)) {
    swfdec_video_decoder_error (dec, "decoding of VP6 data failed");
    return;
  }

  if (swfdec_video_decoder_get_width (vp6->image) != swfdec_video_decoder_get_width (vp6->mask) ||
      swfdec_video_decoder_get_height (vp6->image) != swfdec_video_decoder_get_height (vp6->mask)) {
    SWFDEC_ERROR ("size of mask %ux%u doesn't match image size %ux%u",
	swfdec_video_decoder_get_width (vp6->mask), swfdec_video_decoder_get_height (vp6->mask),
	swfdec_video_decoder_get_width (vp6->image), swfdec_video_decoder_get_height (vp6->image));
    return;
  }
  dec->width = swfdec_video_decoder_get_width (vp6->image);
  dec->height = swfdec_video_decoder_get_height (vp6->image);
  dec->plane[0] = vp6->image->plane[0];
  dec->plane[1] = vp6->image->plane[1];
  dec->plane[2] = vp6->image->plane[2];
  dec->rowstride[0] = vp6->image->rowstride[0];
  dec->rowstride[1] = vp6->image->rowstride[1];
  dec->rowstride[2] = vp6->image->rowstride[2];
  dec->mask = vp6->mask->plane[0];
  dec->mask_rowstride = vp6->mask->rowstride[0];
}

static void
swfdec_video_decoder_vp6_alpha_dispose (GObject *object)
{
  SwfdecVideoDecoderVp6Alpha *vp6 = SWFDEC_VIDEO_DECODER_VP6_ALPHA (object);

  if (vp6->image) {
    g_object_unref (vp6->image);
    vp6->image = NULL;
  }
  if (vp6->mask) {
    g_object_unref (vp6->mask);
    vp6->mask = NULL;
  }

  G_OBJECT_CLASS (swfdec_video_decoder_vp6_alpha_parent_class)->dispose (object);
}

static void
swfdec_video_decoder_vp6_alpha_class_init (SwfdecVideoDecoderVp6AlphaClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecVideoDecoderClass *decoder_class = SWFDEC_VIDEO_DECODER_CLASS (klass);

  object_class->dispose = swfdec_video_decoder_vp6_alpha_dispose;

  decoder_class->prepare = swfdec_video_decoder_vp6_alpha_prepare;
  decoder_class->create = swfdec_video_decoder_vp6_alpha_create;
  decoder_class->decode = swfdec_video_decoder_vp6_alpha_decode;
}

static void
swfdec_video_decoder_vp6_alpha_init (SwfdecVideoDecoderVp6Alpha *vp6)
{
  vp6->image = swfdec_video_decoder_new (SWFDEC_VIDEO_CODEC_VP6, NULL);
  vp6->mask = swfdec_video_decoder_new (SWFDEC_VIDEO_CODEC_VP6, NULL);

  if (swfdec_video_decoder_get_error (vp6->image) ||
      swfdec_video_decoder_get_error (vp6->mask)) {
    swfdec_video_decoder_error (SWFDEC_VIDEO_DECODER (vp6),
	"error initializing children VP6 decoders");
  }
}

