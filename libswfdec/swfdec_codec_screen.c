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

#include "swfdec_codec.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"

typedef struct _SwfdecCodecScreen SwfdecCodecScreen;

struct _SwfdecCodecScreen {
  gulong		width;		/* width of last image */
  gulong		height;		/* height of last image */
  SwfdecBuffer *	buffer;		/* buffer containing last decoded image */
};

static gpointer
swfdec_codec_screen_init (void)
{
  SwfdecCodecScreen *screen = g_new0 (SwfdecCodecScreen, 1);

  return screen;
}

static gboolean
swfdec_codec_screen_get_size (gpointer codec_data,
    unsigned int *width, unsigned int *height)
{
  SwfdecCodecScreen *screen = codec_data;

  if (screen->width == 0 || screen->height == 0)
    return FALSE;

  *width = screen->width;
  *height = screen->height;
  return TRUE;
}
#include <unistd.h>
static SwfdecBuffer *
swfdec_codec_screen_decode (gpointer codec_data, SwfdecBuffer *buffer)
{
  SwfdecCodecScreen *screen = codec_data;
  SwfdecBuffer *ret;
  SwfdecBits bits;
  gulong i, j, w, h, bw, bh, stride;

  swfdec_bits_init (&bits, buffer);
  bw = (swfdec_bits_getbits (&bits, 4) + 1) * 16;
  w = swfdec_bits_getbits (&bits, 12);
  bh = (swfdec_bits_getbits (&bits, 4) + 1) * 16;
  h = swfdec_bits_getbits (&bits, 12);
  if (screen->width == 0 || screen->height == 0) {
    if (w == 0 || h == 0) {
      SWFDEC_ERROR ("width or height is 0: %lux%lu", w, h);
      return NULL;
    }
    screen->width = w;
    screen->height = h;
  } else if (screen->width != w || screen->height != h) {
    SWFDEC_ERROR ("width or height differ from original: was %lux%lu, is %lux%lu",
	screen->width, screen->height, w, h);
    /* FIXME: this is was ffmpeg does, should we be more forgiving? */
    return NULL;
  }
  if (screen->buffer && screen->buffer->ref_count == 1) {
    g_assert (screen->buffer->length == w * h * 4);
    swfdec_buffer_ref (screen->buffer);
    ret = screen->buffer;
  } else {
    ret = swfdec_buffer_new_and_alloc (w * h * 4);
    if (screen->buffer) {
      g_assert (screen->buffer->length == w * h * 4);
      memcpy (ret->data, screen->buffer->data, screen->buffer->length);
      swfdec_buffer_unref (screen->buffer);
    }
    swfdec_buffer_ref (ret);
    screen->buffer = ret;
  }
  stride = w * 4;
  SWFDEC_LOG ("size: %lu x %lu - block size %lu x %lu\n", w, h, bw, bh);
  for (j = 0; j < h; j += bh) {
    for (i = 0; i < w; i += bw) {
      guint x, y, size;
      SwfdecBuffer *buffer;
      guint8 *in, *out;
      size = swfdec_bits_get_bu16 (&bits);
      if (size == 0)
	continue;
      buffer = swfdec_bits_decompress (&bits, size, bw * bh * 4);
      if (buffer == NULL) {
	SWFDEC_WARNING ("error decoding block");
	continue;
      }
      /* convert format and write out data */
      out = ret->data + stride * (h - j - 1) + i * 4;
      in = buffer->data;
      for (y = 0; y < MIN (bh, h - j); y++) {
	for (x = 0; x < MIN (bw, w - i); x++) {
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_BLUE] = *in++;
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_GREEN] = *in++;
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_RED] = *in++;
	  out[x * 4 - y * stride + SWFDEC_COLOR_INDEX_ALPHA] = 0xFF;
	}
      }
    }
  }
  return ret;
}

static void
swfdec_codec_screen_finish (gpointer codec_data)
{
  SwfdecCodecScreen *screen = codec_data;

  if (screen->buffer)
    swfdec_buffer_unref (screen->buffer);
  g_free (screen);
}

const SwfdecVideoCodec swfdec_codec_screen = {
  swfdec_codec_screen_init,
  swfdec_codec_screen_get_size,
  swfdec_codec_screen_decode,
  swfdec_codec_screen_finish
};
