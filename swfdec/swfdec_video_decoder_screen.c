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

#include "swfdec_video_decoder_screen.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecVideoDecoderScreen, swfdec_video_decoder_screen, SWFDEC_TYPE_VIDEO_DECODER)

static gboolean
swfdec_video_decoder_screen_prepare (guint codec, char **missing)
{
  return codec == SWFDEC_VIDEO_CODEC_SCREEN;
}

static SwfdecVideoDecoder *
swfdec_video_decoder_screen_create (guint codec)
{
  if (codec != SWFDEC_VIDEO_CODEC_SCREEN)
    return NULL;

  return g_object_new (SWFDEC_TYPE_VIDEO_DECODER_SCREEN, NULL);
}

static void
swfdec_video_decoder_screen_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecBits bits;
  guint i, j, w, h, bw, bh, stride;

  swfdec_bits_init (&bits, buffer);
  bw = (swfdec_bits_getbits (&bits, 4) + 1) * 16;
  w = swfdec_bits_getbits (&bits, 12);
  bh = (swfdec_bits_getbits (&bits, 4) + 1) * 16;
  h = swfdec_bits_getbits (&bits, 12);
  if (dec->width == 0 || dec->height == 0) {
    if (w == 0 || h == 0) {
      swfdec_video_decoder_error (dec, "width or height is 0: %ux%u", w, h);
      return;
    }
    /* check for overflow */
    if (w * 4 * h < w * 4) {
      swfdec_video_decoder_error (dec, "overflowing video size: %ux%u", w, h);
      return;
    }
    dec->plane[0] = g_try_malloc (w * h * 4);
    if (dec->plane[0] == NULL) {
      swfdec_video_decoder_error (dec, "could not allocate %u bytes", w * h * 4);
      return;
    }
    dec->width = w;
    dec->height = h;
    dec->rowstride[0] = w * 4;
  } else if (dec->width != w || dec->height != h) {
    swfdec_video_decoder_error (dec, "width or height differ from original: was %ux%u, is %ux%u",
	dec->width, dec->height, w, h);
    /* FIXME: this is what ffmpeg does, should we be more forgiving? */
    return;
  }
  stride = w * 4;
  SWFDEC_LOG ("size: %u x %u - block size %u x %u\n", w, h, bw, bh);
  for (j = 0; j < h; j += bh) {
    for (i = 0; i < w; i += bw) {
      guint x, y, size;
      SwfdecBuffer *buf;
      guint8 *in, *out;
      size = swfdec_bits_get_bu16 (&bits);
      if (size == 0)
	continue;
      buf = swfdec_bits_decompress (&bits, size, bw * bh * 4);
      if (buf == NULL) {
	SWFDEC_ERROR ("could not decode block at %ux%u", i, j);
	continue;
      }
      /* convert format and write out data */
      out = dec->plane[0] + stride * (h - j - 1) + i * 4;
      in = buf->data;
      for (y = 0; y < MIN (bh, h - j); y++) {
	for (x = 0; x < MIN (bw, w - i); x++) {
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_BLUE] = *in++;
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_GREEN] = *in++;
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_RED] = *in++;
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_ALPHA] = 0xFF;
	}
      }
      swfdec_buffer_unref (buf);
    }
  }
}

static void
swfdec_video_decoder_screen_dispose (GObject *object)
{
  SwfdecVideoDecoder *dec = SWFDEC_VIDEO_DECODER (object);

  g_free (dec->plane[0]);

  G_OBJECT_CLASS (swfdec_video_decoder_screen_parent_class)->dispose (object);
}

static void
swfdec_video_decoder_screen_class_init (SwfdecVideoDecoderScreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecVideoDecoderClass *decoder_class = SWFDEC_VIDEO_DECODER_CLASS (klass);

  object_class->dispose = swfdec_video_decoder_screen_dispose;

  decoder_class->prepare = swfdec_video_decoder_screen_prepare;
  decoder_class->create = swfdec_video_decoder_screen_create;
  decoder_class->decode = swfdec_video_decoder_screen_decode;
}

static void
swfdec_video_decoder_screen_init (SwfdecVideoDecoderScreen *dec)
{
}

