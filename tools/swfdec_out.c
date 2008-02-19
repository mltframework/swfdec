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

#include "swfdec_out.h"

SwfdecOut *
swfdec_out_open (void)
{
  SwfdecOut *out = g_new0 (SwfdecOut, 1);

  out->data = g_malloc (SWFDEC_OUT_INITIAL);
  out->ptr = out->data;
  out->end = out->data + SWFDEC_OUT_INITIAL;

  return out;
}

static void
swfdec_out_syncbits (SwfdecOut *out)
{
  g_return_if_fail (out != NULL);

  if (out->idx > 0) {
    out->ptr++;
    out->idx = 0;
  }
}

SwfdecBuffer *
swfdec_out_close (SwfdecOut *out)
{
  SwfdecBuffer *buffer;
  
  g_return_val_if_fail (out != NULL, NULL);

  swfdec_out_syncbits (out);

  buffer = swfdec_buffer_new_for_data (out->data, out->ptr - out->data);

  g_free (out);

  return buffer;
}

unsigned int
swfdec_out_get_bits (SwfdecOut *out)
{
  g_return_val_if_fail (out != NULL, 0);

  return (out->ptr - out->data) * 8 + out->idx;
}

unsigned int
swfdec_out_left (SwfdecOut *out)
{
  g_return_val_if_fail (out != NULL, 0);

  return (out->end - out->ptr) * 8 - out->idx;
}

void
swfdec_out_ensure_bits (SwfdecOut *out, unsigned int bits)
{
  unsigned int current, taken, needed;

  g_return_if_fail (out != NULL);

  current = swfdec_out_left (out);
  if (current >= bits)
    return;
  taken = out->ptr - out->data;
  needed = (bits - current + 7) / 8;
  needed += SWFDEC_OUT_STEP;
  needed -= needed % SWFDEC_OUT_STEP;
  needed += out->end - out->data;
  out->data = g_realloc (out->data, needed);
  out->ptr = out->data + taken;
  out->end = out->data + needed;
}

void
swfdec_out_prepare_bytes (SwfdecOut *out, unsigned int bytes)
{
  g_return_if_fail (out != NULL);

  swfdec_out_syncbits (out);
  swfdec_out_ensure_bits (out, bytes * 8);
}

void
swfdec_out_put_data (SwfdecOut *out, const guint8 *data, guint length)
{
  g_return_if_fail (out != NULL);

  swfdec_out_prepare_bytes (out, length);
  memcpy (out->ptr, data, length);
  out->ptr += length;
}

void
swfdec_out_put_buffer (SwfdecOut *out, SwfdecBuffer *buffer)
{
  g_return_if_fail (out != NULL);

  swfdec_out_prepare_bytes (out, buffer->length);
  memcpy (out->ptr, buffer->data, buffer->length);
  out->ptr += buffer->length;
}

void
swfdec_out_put_u8 (SwfdecOut *out, guint i)
{
  g_return_if_fail (i <= G_MAXUINT8);

  swfdec_out_prepare_bytes (out, 1);
  *out->ptr = i;
  out->ptr++;
}

void
swfdec_out_put_u16 (SwfdecOut *out, guint i)
{
  g_return_if_fail (i <= G_MAXUINT16);

  swfdec_out_prepare_bytes (out, 2);
  *(guint16 *)out->ptr = GUINT16_TO_LE (i);
  out->ptr += 2;
}

void
swfdec_out_put_u32 (SwfdecOut *out, guint i)
{
  g_return_if_fail (i <= G_MAXUINT32);

  swfdec_out_prepare_bytes (out, 4);
  *(guint32 *)out->ptr = GUINT32_TO_LE (i);
  out->ptr += 4;
}

void
swfdec_out_put_bit (SwfdecOut *out, gboolean bit)
{
  g_return_if_fail (out != NULL);

  swfdec_out_put_bits (out, bit ? 1 : 0, 1);
}

void
swfdec_out_put_bits (SwfdecOut *out, guint bits, guint n_bits)
{
  g_return_if_fail (out != NULL);

  swfdec_out_ensure_bits (out, n_bits);

  /* FIXME: implement this less braindead */
  while (n_bits) {
    guint bits_now = MIN (n_bits, 8 - out->idx);
    guint value = bits >> (n_bits - bits_now);

    /* clear data if necessary */
    if (out->idx == 0)
      *out->ptr = 0;
    value &= (1 << bits_now) - 1;
    value <<= 8 - out->idx - bits_now;
    *out->ptr |= value;
    out->idx += bits_now;
    g_assert (out->idx <= 8);
    if (out->idx == 8) {
      out->ptr ++;
      out->idx = 0;
    }
    n_bits -= bits_now;
  }
}

void
swfdec_out_put_sbits (SwfdecOut *out, int bits, guint n_bits)
{
  g_return_if_fail (out != NULL);
  swfdec_out_put_bits (out, bits, n_bits);
}

void
swfdec_out_put_string (SwfdecOut *out, const char *s)
{
  guint len;

  g_return_if_fail (out != NULL);
  g_return_if_fail (s != NULL);

  len = strlen (s) + 1;

  swfdec_out_prepare_bytes (out, len);
  memcpy (out->ptr, s, len);
  out->ptr += len;
}

static guint
swfdec_out_bits_required (guint x)
{
  guint ret = 0;

  while (x > 0) {
    x >>= 1;
    ret++;
  }
  return ret;
}

static guint
swfdec_out_sbits_required (int x)
{
  if (x < 0)
    x = !x;
  return swfdec_out_bits_required (x) + 1;
}

void
swfdec_out_put_rect (SwfdecOut *out, const SwfdecRect *rect)
{
  int x0, x1, y0, y1;
  guint req, tmp;

  g_return_if_fail (out != NULL);
  g_return_if_fail (rect != NULL);

  x0 = rect->x0;
  y0 = rect->y0;
  x1 = rect->x1;
  y1 = rect->y1;
  req = swfdec_out_sbits_required (x0);
  tmp = swfdec_out_sbits_required (y0);
  req = MAX (req, tmp);
  tmp = swfdec_out_sbits_required (x1);
  req = MAX (req, tmp);
  tmp = swfdec_out_sbits_required (y1);
  req = MAX (req, tmp);
  swfdec_out_syncbits (out);
  swfdec_out_put_bits (out, req, 5);
  swfdec_out_put_sbits (out, x0, req);
  swfdec_out_put_sbits (out, x1, req);
  swfdec_out_put_sbits (out, y0, req);
  swfdec_out_put_sbits (out, y1, req);
  swfdec_out_syncbits (out);
}

void
swfdec_out_put_matrix (SwfdecOut *out, const cairo_matrix_t *matrix)
{
  int x, y;
  unsigned int xbits, ybits;

  if (matrix->xx != 1.0 || matrix->yy != 1.0) {
    swfdec_out_put_bit (out, 1);
    x = SWFDEC_DOUBLE_TO_FIXED (matrix->xx);
    y = SWFDEC_DOUBLE_TO_FIXED (matrix->yy);
    xbits = swfdec_out_sbits_required (x);
    ybits = swfdec_out_sbits_required (y);
    xbits = MAX (xbits, ybits);
    swfdec_out_put_bits (out, xbits, 5);
    swfdec_out_put_sbits (out, x, xbits);
    swfdec_out_put_sbits (out, y, xbits);
  } else {
    swfdec_out_put_bit (out, 0);
  }
  if (matrix->xy != 0.0 || matrix->yx != 0.0) {
    swfdec_out_put_bit (out, 1);
    x = SWFDEC_DOUBLE_TO_FIXED (matrix->yx);
    y = SWFDEC_DOUBLE_TO_FIXED (matrix->xy);
    xbits = swfdec_out_sbits_required (x);
    ybits = swfdec_out_sbits_required (y);
    xbits = MAX (xbits, ybits);
    swfdec_out_put_bits (out, xbits, 5);
    swfdec_out_put_sbits (out, x, xbits);
    swfdec_out_put_sbits (out, y, xbits);
  } else {
    swfdec_out_put_bit (out, 0);
  }
  x = matrix->x0;
  y = matrix->y0;
  xbits = swfdec_out_sbits_required (x);
  ybits = swfdec_out_sbits_required (y);
  xbits = MAX (xbits, ybits);
  swfdec_out_put_bits (out, xbits, 5);
  swfdec_out_put_sbits (out, x, xbits);
  swfdec_out_put_sbits (out, y, xbits);
  swfdec_out_syncbits (out);
}

void
swfdec_out_put_color_transform (SwfdecOut *out, const SwfdecColorTransform *trans)
{
  gboolean has_add, has_mult;
  unsigned int n_bits, tmp;

  has_mult = trans->ra != 256 || trans->ga != 256 || trans->ba != 256 || trans->aa != 256;
  has_add = trans->rb != 0 || trans->gb != 0 || trans->bb != 0 || trans->ab != 0;
  if (has_mult) {
    n_bits = swfdec_out_sbits_required (trans->ra);
    tmp = swfdec_out_sbits_required (trans->ga);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_out_sbits_required (trans->ba);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_out_sbits_required (trans->aa);
    n_bits = MAX (tmp, n_bits);
  } else {
    n_bits = 0;
  }
  if (has_add) {
    tmp = swfdec_out_sbits_required (trans->rb);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_out_sbits_required (trans->gb);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_out_sbits_required (trans->bb);
    n_bits = MAX (tmp, n_bits);
    tmp = swfdec_out_sbits_required (trans->ab);
    n_bits = MAX (tmp, n_bits);
  }
  if (n_bits >= (1 << 4))
    n_bits = (1 << 4) - 1;
  swfdec_out_put_bit (out, has_add);
  swfdec_out_put_bit (out, has_mult);
  swfdec_out_put_bits (out, n_bits, 4);
  if (has_mult) {
    swfdec_out_put_sbits (out, trans->ra, n_bits);
    swfdec_out_put_sbits (out, trans->ga, n_bits);
    swfdec_out_put_sbits (out, trans->ba, n_bits);
    swfdec_out_put_sbits (out, trans->aa, n_bits);
  }
  if (has_add) {
    swfdec_out_put_sbits (out, trans->rb, n_bits);
    swfdec_out_put_sbits (out, trans->gb, n_bits);
    swfdec_out_put_sbits (out, trans->bb, n_bits);
    swfdec_out_put_sbits (out, trans->ab, n_bits);
  }
  swfdec_out_syncbits (out);
}

void
swfdec_out_put_rgb (SwfdecOut *out, SwfdecColor color)
{
  g_return_if_fail (out != NULL);

  swfdec_out_put_u8 (out, SWFDEC_COLOR_R (color));
  swfdec_out_put_u8 (out, SWFDEC_COLOR_G (color));
  swfdec_out_put_u8 (out, SWFDEC_COLOR_B (color));
}

void
swfdec_out_put_rgba (SwfdecOut *out, SwfdecColor color)
{
  g_return_if_fail (out != NULL);

  swfdec_out_put_u8 (out, SWFDEC_COLOR_R (color));
  swfdec_out_put_u8 (out, SWFDEC_COLOR_G (color));
  swfdec_out_put_u8 (out, SWFDEC_COLOR_B (color));
  swfdec_out_put_u8 (out, SWFDEC_COLOR_A (color));
}

