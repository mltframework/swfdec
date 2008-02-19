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

#include <string.h>
#include <zlib.h>
#include <liboil/liboil.h>

#include "swfdec_codec_video.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

typedef struct _SwfdecCodecVp6Alpha SwfdecCodecVp6Alpha;

struct _SwfdecCodecVp6Alpha {
  SwfdecVideoDecoder	decoder;	/* the decoder */
  SwfdecVideoDecoder *	image;		/* the image decoder */
  SwfdecVideoDecoder *	alpha;		/* the alpha decoder */
};

static gboolean
swfdec_video_decoder_vp6_alpha_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer,
    SwfdecVideoImage *image)
{
  SwfdecCodecVp6Alpha *vp6 = (SwfdecCodecVp6Alpha *) dec;
  SwfdecBuffer *tmp;
  SwfdecVideoImage alpha;
  SwfdecBits bits;
  guint size;

  swfdec_bits_init (&bits, buffer);
  size = swfdec_bits_get_bu24 (&bits);
  tmp = swfdec_bits_get_buffer (&bits, size);
  if (tmp == NULL)
    return FALSE;
  if (!vp6->image->decode (vp6->image, tmp, image)) {
    swfdec_buffer_unref (tmp);
    return FALSE;
  }
  swfdec_buffer_unref (tmp);
  tmp = swfdec_bits_get_buffer (&bits, -1);
  if (tmp == NULL)
    return FALSE;
  if (!vp6->alpha->decode (vp6->alpha, tmp, &alpha)) {
    swfdec_buffer_unref (tmp);
    return FALSE;
  }
  swfdec_buffer_unref (tmp);
  if (alpha.width != image->width || alpha.height != image->height) {
    SWFDEC_ERROR ("size of mask doesn't match image: %ux%u vs %ux%u", 
	alpha.width, alpha.height, image->width, image->height);
    return FALSE;
  }
  image->mask = alpha.plane[0];
  image->mask_rowstride = alpha.rowstride[0];
  return TRUE;
}

static void
swfdec_video_decoder_vp6_alpha_free (SwfdecVideoDecoder *dec)
{
  SwfdecCodecVp6Alpha *vp6 = (SwfdecCodecVp6Alpha *) dec;

  if (vp6->image)
    swfdec_video_decoder_free (vp6->image);
  if (vp6->alpha)
    swfdec_video_decoder_free (vp6->alpha);
  g_free (vp6);
}

SwfdecVideoDecoder *
swfdec_video_decoder_vp6_alpha_new (guint type)
{
  SwfdecCodecVp6Alpha *vp6;
  
  if (type != SWFDEC_VIDEO_CODEC_VP6_ALPHA)
    return NULL;
  
  vp6 = g_new0 (SwfdecCodecVp6Alpha, 1);
  vp6->decoder.decode = swfdec_video_decoder_vp6_alpha_decode;
  vp6->decoder.free = swfdec_video_decoder_vp6_alpha_free;
  vp6->image = swfdec_video_decoder_new (SWFDEC_VIDEO_CODEC_VP6);
  vp6->alpha = swfdec_video_decoder_new (SWFDEC_VIDEO_CODEC_VP6);
  if (vp6->alpha == NULL || vp6->image == NULL) {
    swfdec_video_decoder_vp6_alpha_free (&vp6->decoder);
    return NULL;
  }

  return &vp6->decoder;
}

