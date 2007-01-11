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
  guint			width;		/* width of last image */
  guint			height;		/* height of last image */
  SwfdecBuffer *	buffer;		/* buffer containing last decoded image */
  z_stream		z;		/* stream for decoding */
  SwfdecBuffer *	block_buffer;	/* buffer used for block decoding */
};

static void *
zalloc (void *opaque, unsigned int items, unsigned int size)
{
  return g_malloc (items * size);
}

static void
zfree (void *opaque, void *addr)
{
  g_free (addr);
}

static gpointer
swfdec_codec_screen_init (void)
{
  SwfdecCodecScreen *screen = g_new0 (SwfdecCodecScreen, 1);

  screen->z.zalloc = zalloc;
  screen->z.zfree = zfree;
  screen->z.opaque = NULL;
  if (inflateInit (&screen->z) != Z_OK) {
    SWFDEC_ERROR ("Error initialising zlib: %s", screen->z.msg);
    g_free (screen);
    return NULL;
  }
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
  guint i, j, w, h, bw, bh, stride;

  swfdec_bits_init (&bits, buffer);
  bw = (swfdec_bits_getbits (&bits, 4) + 1) * 16;
  w = swfdec_bits_getbits (&bits, 12);
  bh = (swfdec_bits_getbits (&bits, 4) + 1) * 16;
  h = swfdec_bits_getbits (&bits, 12);
  if (screen->width == 0 || screen->height == 0) {
    if (w == 0 || h == 0) {
      SWFDEC_ERROR ("width or height is 0: %ux%u", w, h);
      return NULL;
    }
    screen->width = w;
    screen->height = h;
  } else if (screen->width != w || screen->height != h) {
    SWFDEC_ERROR ("width or height differ from original: was %ux%u, is %ux%u",
	screen->width, screen->height, w, h);
    /* FIXME: this is was ffmpeg does, should we be more forgiving? */
    return NULL;
  }
  if (screen->block_buffer == NULL || screen->block_buffer->length < bw * bh * 3) {
    if (screen->block_buffer != NULL)
      swfdec_buffer_unref (screen->block_buffer);
    screen->block_buffer = swfdec_buffer_new_and_alloc (bw * bh * 3);
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
  SWFDEC_LOG ("size: %u x %u - block size %u x %u\n", w, h, bw, bh);
  for (j = 0; j < h; j += bh) {
    for (i = 0; i < w; i += bw) {
      int result;
      guint x, y, size;
      guint8 *in, *out;
      /* decode the block into block_buffer */
      size = swfdec_bits_get_bu16 (&bits);
      if (size == 0)
	continue;
      if (inflateReset(&screen->z) != Z_OK) {
	SWFDEC_ERROR ("error resetting zlib decoder: %s", screen->z.msg);
      }
      screen->z.next_in = bits.ptr;
      if (swfdec_bits_skip_bytes (&bits, size) != size) {
	SWFDEC_ERROR ("not enough bytes available");
	return NULL;
      }
      screen->z.avail_in = size;
      screen->z.next_out = screen->block_buffer->data;
      screen->z.avail_out = screen->block_buffer->length;
      result = inflate (&screen->z, Z_FINISH);
      if (result == Z_DATA_ERROR) {
	inflateSync (&screen->z);
	result = inflate (&screen->z, Z_FINISH);
      }
      if (result < 0) {
	SWFDEC_WARNING ("error decoding block: %s", screen->z.msg);
	continue;
      }
      /* convert format and write out data */
      out = ret->data + stride * (h - j - 1) + i * 4;
      in = screen->block_buffer->data;
      for (y = 0; y < MIN (bh, h - j); y++) {
	for (x = 0; x < MIN (bw, w - i); x++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	  out[x * 4 - y * stride] = *in++;
	  out[x * 4 - y * stride + 1] = *in++;
	  out[x * 4 - y * stride + 2] = *in++;
	  out[x * 4 - y * stride + 3] = 0xFF;
#else
	  out[x * 4 - y * stride + 3] = *in++;
	  out[x * 4 - y * stride + 2] = *in++;
	  out[x * 4 - y * stride + 1] = *in++;
	  out[x * 4 - y * stride] = 0xFF;
#endif
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
