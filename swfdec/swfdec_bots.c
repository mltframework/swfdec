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

#include "swfdec_bots.h"

SwfdecBots *
swfdec_bots_open (void)
{
  SwfdecBots *bots = g_new0 (SwfdecBots, 1);

  bots->data = g_malloc (SWFDEC_OUT_INITIAL);
  bots->ptr = bots->data;
  bots->end = bots->data + SWFDEC_OUT_INITIAL;

  return bots;
}

static void
swfdec_bots_syncbits (SwfdecBots *bots)
{
  g_return_if_fail (bots != NULL);

  if (bots->idx > 0) {
    bots->ptr++;
    bots->idx = 0;
  }
}

SwfdecBuffer *
swfdec_bots_close (SwfdecBots *bots)
{
  SwfdecBuffer *buffer;
  
  g_return_val_if_fail (bots != NULL, NULL);

  swfdec_bots_syncbits (bots);

  buffer = swfdec_buffer_new_for_data (bots->data, bots->ptr - bots->data);

  g_free (bots);

  return buffer;
}

gsize
swfdec_bots_get_bits (SwfdecBots *bots)
{
  g_return_val_if_fail (bots != NULL, 0);

  return (bots->ptr - bots->data) * 8 + bots->idx;
}

gsize
swfdec_bots_get_bytes (SwfdecBots *bots)
{
  g_return_val_if_fail (bots != NULL, 0);

  g_assert (bots->idx == 0);

  return swfdec_bots_get_bits (bots) / 8;
}

gsize
swfdec_bots_left (SwfdecBots *bots)
{
  g_return_val_if_fail (bots != NULL, 0);

  return (bots->end - bots->ptr) * 8 - bots->idx;
}

void
swfdec_bots_ensure_bits (SwfdecBots *bots, unsigned int bits)
{
  unsigned int current, taken, needed;

  g_return_if_fail (bots != NULL);

  current = swfdec_bots_left (bots);
  if (current >= bits)
    return;
  taken = bots->ptr - bots->data;
  needed = (bits - current + 7) / 8;
  needed += SWFDEC_OUT_STEP;
  needed -= needed % SWFDEC_OUT_STEP;
  needed += bots->end - bots->data;
  bots->data = g_realloc (bots->data, needed);
  bots->ptr = bots->data + taken;
  bots->end = bots->data + needed;
}

void
swfdec_bots_prepare_bytes (SwfdecBots *bots, unsigned int bytes)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_syncbits (bots);
  swfdec_bots_ensure_bits (bots, bytes * 8);
}

void
swfdec_bots_put_data (SwfdecBots *bots, const guint8 *data, guint length)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_prepare_bytes (bots, length);
  memcpy (bots->ptr, data, length);
  bots->ptr += length;
}

void
swfdec_bots_put_buffer (SwfdecBots *bots, SwfdecBuffer *buffer)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_prepare_bytes (bots, buffer->length);
  memcpy (bots->ptr, buffer->data, buffer->length);
  bots->ptr += buffer->length;
}

void
swfdec_bots_put_bots (SwfdecBots *bots, SwfdecBots *other)
{
  gsize bytes;

  g_return_if_fail (bots != NULL);
  g_return_if_fail (other != NULL);

  bytes = swfdec_bots_get_bytes (other);
  swfdec_bots_prepare_bytes (bots, bytes);
  memcpy (bots->ptr, other->data, bytes);
  bots->ptr += bytes;
}

void
swfdec_bots_put_u8 (SwfdecBots *bots, guint i)
{
  g_return_if_fail (i <= G_MAXUINT8);

  swfdec_bots_prepare_bytes (bots, 1);
  *bots->ptr = i;
  bots->ptr++;
}

void
swfdec_bots_put_u16 (SwfdecBots *bots, guint i)
{
  g_return_if_fail (i <= G_MAXUINT16);

  swfdec_bots_prepare_bytes (bots, 2);
  *(guint16 *)bots->ptr = GUINT16_TO_LE (i);
  bots->ptr += 2;
}

void
swfdec_bots_put_s16 (SwfdecBots *bots, int i)
{
  g_return_if_fail (i >= G_MININT16 && i <= G_MAXINT16);

  swfdec_bots_prepare_bytes (bots, 2);
  *(guint16 *)bots->ptr = GINT16_TO_LE (i);
  bots->ptr += 2;
}

void
swfdec_bots_put_u32 (SwfdecBots *bots, guint i)
{
  g_return_if_fail (i <= G_MAXUINT32);

  swfdec_bots_prepare_bytes (bots, 4);
  *(guint32 *)bots->ptr = GUINT32_TO_LE (i);
  bots->ptr += 4;
}

void
swfdec_bots_put_bit (SwfdecBots *bots, gboolean bit)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_put_bits (bots, bit ? 1 : 0, 1);
}

void
swfdec_bots_put_bits (SwfdecBots *bots, guint bits, guint n_bits)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_ensure_bits (bots, n_bits);

  /* FIXME: implement this less braindead */
  while (n_bits) {
    guint bits_now = MIN (n_bits, 8 - bots->idx);
    guint value = bits >> (n_bits - bits_now);

    /* clear data if necessary */
    if (bots->idx == 0)
      *bots->ptr = 0;
    value &= (1 << bits_now) - 1;
    value <<= 8 - bots->idx - bits_now;
    *bots->ptr |= value;
    bots->idx += bits_now;
    g_assert (bots->idx <= 8);
    if (bots->idx == 8) {
      bots->ptr ++;
      bots->idx = 0;
    }
    n_bits -= bits_now;
  }
}

void
swfdec_bots_put_sbits (SwfdecBots *bots, int bits, guint n_bits)
{
  g_return_if_fail (bots != NULL);
  swfdec_bots_put_bits (bots, bits, n_bits);
}

void
swfdec_bots_put_string (SwfdecBots *bots, const char *s)
{
  guint len;

  g_return_if_fail (bots != NULL);
  g_return_if_fail (s != NULL);

  len = strlen (s) + 1;

  swfdec_bots_prepare_bytes (bots, len);
  memcpy (bots->ptr, s, len);
  bots->ptr += len;
}

 /* If little endian x86 byte order is 0 1 2 3 4 5 6 7 and PPC32 byte order is
 * 7 6 5 4 3 2 1 0, then Flash uses 4 5 6 7 0 1 2 3. */
void
swfdec_bots_put_double (SwfdecBots *bots, double value)
{
  union {
    guint32 i[2];
    double d;
  } conv;

  swfdec_bots_ensure_bits (bots, 8);

  conv.d = value;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  swfdec_bots_put_u32 (bots, conv.i[1]);
  swfdec_bots_put_u32 (bots, conv.i[0]);
#else
  swfdec_bots_put_u32 (bots, GUINT32_TO_LE (conv.i[0]));
  swfdec_bots_put_u32 (bots, GUINT32_TO_LE (conv.i[1]));
#endif
}

static guint
swfdec_bots_bits_required (guint x)
{
  guint ret = 0;

  while (x > 0) {
    x >>= 1;
    ret++;
  }
  return ret;
}

static guint
swfdec_bots_sbits_required (int x)
{
  if (x < 0)
    x = !x;
  return swfdec_bots_bits_required (x) + 1;
}

void
swfdec_bots_put_rect (SwfdecBots *bots, const SwfdecRect *rect)
{
  int x0, x1, y0, y1;
  guint req, tmp;

  g_return_if_fail (bots != NULL);
  g_return_if_fail (rect != NULL);

  x0 = rect->x0;
  y0 = rect->y0;
  x1 = rect->x1;
  y1 = rect->y1;
  req = swfdec_bots_sbits_required (x0);
  tmp = swfdec_bots_sbits_required (y0);
  req = MAX (req, tmp);
  tmp = swfdec_bots_sbits_required (x1);
  req = MAX (req, tmp);
  tmp = swfdec_bots_sbits_required (y1);
  req = MAX (req, tmp);
  swfdec_bots_syncbits (bots);
  swfdec_bots_put_bits (bots, req, 5);
  swfdec_bots_put_sbits (bots, x0, req);
  swfdec_bots_put_sbits (bots, x1, req);
  swfdec_bots_put_sbits (bots, y0, req);
  swfdec_bots_put_sbits (bots, y1, req);
  swfdec_bots_syncbits (bots);
}

void
swfdec_bots_put_matrix (SwfdecBots *bots, const cairo_matrix_t *matrix)
{
  int x, y;
  unsigned int xbits, ybits;

  if (matrix->xx != 1.0 || matrix->yy != 1.0) {
    swfdec_bots_put_bit (bots, 1);
    x = SWFDEC_DOUBLE_TO_FIXED (matrix->xx);
    y = SWFDEC_DOUBLE_TO_FIXED (matrix->yy);
    xbits = swfdec_bots_sbits_required (x);
    ybits = swfdec_bots_sbits_required (y);
    xbits = MAX (xbits, ybits);
    swfdec_bots_put_bits (bots, xbits, 5);
    swfdec_bots_put_sbits (bots, x, xbits);
    swfdec_bots_put_sbits (bots, y, xbits);
  } else {
    swfdec_bots_put_bit (bots, 0);
  }
  if (matrix->xy != 0.0 || matrix->yx != 0.0) {
    swfdec_bots_put_bit (bots, 1);
    x = SWFDEC_DOUBLE_TO_FIXED (matrix->yx);
    y = SWFDEC_DOUBLE_TO_FIXED (matrix->xy);
    xbits = swfdec_bots_sbits_required (x);
    ybits = swfdec_bots_sbits_required (y);
    xbits = MAX (xbits, ybits);
    swfdec_bots_put_bits (bots, xbits, 5);
    swfdec_bots_put_sbits (bots, x, xbits);
    swfdec_bots_put_sbits (bots, y, xbits);
  } else {
    swfdec_bots_put_bit (bots, 0);
  }
  x = matrix->x0;
  y = matrix->y0;
  xbits = swfdec_bots_sbits_required (x);
  ybits = swfdec_bots_sbits_required (y);
  xbits = MAX (xbits, ybits);
  swfdec_bots_put_bits (bots, xbits, 5);
  swfdec_bots_put_sbits (bots, x, xbits);
  swfdec_bots_put_sbits (bots, y, xbits);
  swfdec_bots_syncbits (bots);
}

void
swfdec_bots_put_color_transform (SwfdecBots *bots, const SwfdecColorTransform *trans)
{
  gboolean has_add, has_mult;
  unsigned int n_bits, tmp;

  has_mult = trans->ra != 256 || trans->ga != 256 || trans->ba != 256 || trans->aa != 256;
  has_add = trans->rb != 0 || trans->gb != 0 || trans->bb != 0 || trans->ab != 0;
  if (has_mult) {
    n_bits = swfdec_bots_sbits_required (trans->ra);
    tmp = swfdec_bots_sbits_required (trans->ga);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_bots_sbits_required (trans->ba);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_bots_sbits_required (trans->aa);
    n_bits = MAX (tmp, n_bits);
  } else {
    n_bits = 0;
  }
  if (has_add) {
    tmp = swfdec_bots_sbits_required (trans->rb);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_bots_sbits_required (trans->gb);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_bots_sbits_required (trans->bb);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_bots_sbits_required (trans->ab);
    n_bits = MAX (tmp, n_bits);
  }
  if (n_bits >= (1 << 4))
    n_bits = (1 << 4) - 1;
  swfdec_bots_put_bit (bots, has_add);
  swfdec_bots_put_bit (bots, has_mult);
  swfdec_bots_put_bits (bots, n_bits, 4);
  if (has_mult) {
    swfdec_bots_put_sbits (bots, trans->ra, n_bits);
    swfdec_bots_put_sbits (bots, trans->ga, n_bits);
    swfdec_bots_put_sbits (bots, trans->ba, n_bits);
    swfdec_bots_put_sbits (bots, trans->aa, n_bits);
  }
  if (has_add) {
    swfdec_bots_put_sbits (bots, trans->rb, n_bits);
    swfdec_bots_put_sbits (bots, trans->gb, n_bits);
    swfdec_bots_put_sbits (bots, trans->bb, n_bits);
    swfdec_bots_put_sbits (bots, trans->ab, n_bits);
  }
  swfdec_bots_syncbits (bots);
}

void
swfdec_bots_put_rgb (SwfdecBots *bots, SwfdecColor color)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_R (color));
  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_G (color));
  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_B (color));
}

void
swfdec_bots_put_rgba (SwfdecBots *bots, SwfdecColor color)
{
  g_return_if_fail (bots != NULL);

  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_R (color));
  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_G (color));
  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_B (color));
  swfdec_bots_put_u8 (bots, SWFDEC_COLOR_A (color));
}

