/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_rect.h"


void 
swfdec_bits_init (SwfdecBits *bits, SwfdecBuffer *buffer)
{
  g_return_if_fail (bits != NULL);
  g_return_if_fail (buffer != NULL);

  bits->buffer = buffer;
  bits->ptr = buffer->data;
  bits->idx = 0;
  bits->end = buffer->data + buffer->length;
}

unsigned int 
swfdec_bits_left (SwfdecBits *b)
{
  if (b->ptr == NULL)
    return 0;
  g_assert (b->end >= b->ptr);
  g_assert (b->end > b->ptr || b->idx == 0);
  return (b->end - b->ptr) * 8 - b->idx;
}

#define SWFDEC_BITS_CHECK(b,n) G_STMT_START { \
  if (swfdec_bits_left(b) < (n)) { \
    SWFDEC_ERROR("reading past end of buffer"); \
    return 0; \
  } \
}G_STMT_END
#define SWFDEC_BYTES_CHECK(b,n) G_STMT_START { \
  swfdec_bits_syncbits (b); \
  SWFDEC_BITS_CHECK (b, 8 * n); \
} G_STMT_END

int
swfdec_bits_getbit (SwfdecBits * b)
{
  int r;

  SWFDEC_BITS_CHECK (b, 1);

  r = ((*b->ptr) >> (7 - b->idx)) & 1;

  b->idx++;
  if (b->idx >= 8) {
    b->ptr++;
    b->idx = 0;
  }

  return r;
}

unsigned int
swfdec_bits_getbits (SwfdecBits * b, unsigned int n)
{
  unsigned long r = 0;
  unsigned int i;

  SWFDEC_BITS_CHECK (b, n);

  while (n > 0) {
    i = MIN (n, 8 - b->idx);
    r <<= i;
    r |= ((*b->ptr) >> (8 - i - b->idx)) & ((1 << i) - 1);
    n -= i;
    if (i == 8) {
      b->ptr++;
    } else {
      b->idx += i;
      if (b->idx >= 8) {
	b->ptr++;
	b->idx = 0;
      }
    }
  }
  return r;
}

unsigned int
swfdec_bits_peekbits (SwfdecBits * b, unsigned int n)
{
  SwfdecBits tmp = *b;

  return swfdec_bits_getbits (&tmp, n);
}

int
swfdec_bits_getsbits (SwfdecBits * b, unsigned int n)
{
  unsigned long r = 0;

  SWFDEC_BITS_CHECK (b, n);

  if (n == 0)
    return 0;
  r = -swfdec_bits_getbit (b);
  r = (r << (n - 1)) | swfdec_bits_getbits (b, n - 1);
  return r;
}

unsigned int
swfdec_bits_peek_u8 (SwfdecBits * b)
{
  SWFDEC_BYTES_CHECK (b, 1);

  return *b->ptr;
}

unsigned int
swfdec_bits_get_u8 (SwfdecBits * b)
{
  SWFDEC_BYTES_CHECK (b, 1);

  return *b->ptr++;
}

unsigned int
swfdec_bits_get_u16 (SwfdecBits * b)
{
  unsigned int r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = b->ptr[0] | (b->ptr[1] << 8);
  b->ptr += 2;

  return r;
}

int
swfdec_bits_get_s16 (SwfdecBits * b)
{
  short r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = b->ptr[0] | (b->ptr[1] << 8);
  b->ptr += 2;

  return r;
}

unsigned int
swfdec_bits_get_be_u16 (SwfdecBits * b)
{
  unsigned int r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = (b->ptr[0] << 8) | b->ptr[1];
  b->ptr += 2;

  return r;
}

unsigned int
swfdec_bits_get_u32 (SwfdecBits * b)
{
  unsigned int r;

  SWFDEC_BYTES_CHECK (b, 4);

  r = b->ptr[0] | (b->ptr[1] << 8) | (b->ptr[2] << 16) | (b->ptr[3] << 24);
  b->ptr += 4;

  return r;
}

float
swfdec_bits_get_float (SwfdecBits * b)
{
  union {
    gint32 i;
    float f;
  } conv;

  SWFDEC_BYTES_CHECK (b, 4);

  conv.i = *((gint32 *) b->ptr);
  b->ptr += 4;

  conv.i = GINT32_FROM_LE (conv.i);

  return conv.f;
}

/* fixup mad byte ordering of doubles in flash files.
 * If little endian x86 byte order is 0 1 2 3 4 5 6 7 and PPC32 byte order is
 * 7 6 5 4 3 2 1 0, then Flash uses 4 5 6 7 0 1 2 3.
 * If your architecture has a different byte ordering for storing doubles,
 * this conversion function will fail. To find out your byte ordering, you can
 * use this command line:
 * python -c "import struct; print struct.unpack('8c', struct.pack('d', 7.949928895127363e-275))"
 */
static double 
swfdec_bits_double_to_host (double in)
{
  union {
    guint32 i[2];
    double d;
  } conv;

  conv.d = in;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  {
    int tmp = conv.i[0];
    conv.i[0] = conv.i[1];
    conv.i[1] = tmp;
  }
#else
  conv.i[0] = GUINT32_FROM_LE (conv.i[0]);
  conv.i[1] = GUINT32_FROM_LE (conv.i[1]);
#endif
  return conv.d;
}

double
swfdec_bits_get_double (SwfdecBits * b)
{
  double d;

  SWFDEC_BYTES_CHECK (b, 8);

  d = *((double *) b->ptr);
  b->ptr += 8;
  d = swfdec_bits_double_to_host (d);

  return d;
}

void
swfdec_bits_syncbits (SwfdecBits * b)
{
  if (b->idx) {
    b->ptr++;
    b->idx = 0;
  }
}

void
swfdec_bits_get_color_transform (SwfdecBits * bits, SwfdecColorTransform * ct)
{
  int has_add;
  int has_mult;
  int n_bits;

  swfdec_bits_syncbits (bits);
  has_add = swfdec_bits_getbit (bits);
  has_mult = swfdec_bits_getbit (bits);
  n_bits = swfdec_bits_getbits (bits, 4);
  if (has_mult) {
    ct->ra = swfdec_bits_getsbits (bits, n_bits);
    ct->ga = swfdec_bits_getsbits (bits, n_bits);
    ct->ba = swfdec_bits_getsbits (bits, n_bits);
    ct->aa = swfdec_bits_getsbits (bits, n_bits);
  } else {
    ct->ra = 256;
    ct->ga = 256;
    ct->ba = 256;
    ct->aa = 256;
  }
  if (has_add) {
    ct->rb = swfdec_bits_getsbits (bits, n_bits);
    ct->gb = swfdec_bits_getsbits (bits, n_bits);
    ct->bb = swfdec_bits_getsbits (bits, n_bits);
    ct->ab = swfdec_bits_getsbits (bits, n_bits);
  } else {
    ct->rb = 0;
    ct->gb = 0;
    ct->bb = 0;
    ct->ab = 0;
  }
}

void
swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix,
    cairo_matrix_t *inverse)
{
  int has_scale;
  int has_rotate;
  int n_translate_bits;

  swfdec_bits_syncbits (bits);

  has_scale = swfdec_bits_getbit (bits);
  if (has_scale) {
    int n_scale_bits = swfdec_bits_getbits (bits, 5);
    matrix->xx = SWFDEC_FIXED_TO_DOUBLE (swfdec_bits_getsbits (bits, n_scale_bits));
    matrix->yy = SWFDEC_FIXED_TO_DOUBLE (swfdec_bits_getsbits (bits, n_scale_bits));

    SWFDEC_LOG ("scalefactors: x = %g, y = %g", matrix->xx, matrix->yy);
  } else {
    SWFDEC_LOG ("no scalefactors given");
    matrix->xx = 1.0;
    matrix->yy = 1.0;
  }
  has_rotate = swfdec_bits_getbit (bits);
  if (has_rotate) {
    int n_rotate_bits = swfdec_bits_getbits (bits, 5);
    matrix->yx = SWFDEC_FIXED_TO_DOUBLE (swfdec_bits_getsbits (bits, n_rotate_bits));
    matrix->xy = SWFDEC_FIXED_TO_DOUBLE (swfdec_bits_getsbits (bits, n_rotate_bits));

    SWFDEC_LOG ("skew: xy = %g, yx = %g", matrix->xy, matrix->yx);
  } else {
    SWFDEC_LOG ("no rotation");
    matrix->xy = 0;
    matrix->yx = 0;
  }
  n_translate_bits = swfdec_bits_getbits (bits, 5);
  matrix->x0 = swfdec_bits_getsbits (bits, n_translate_bits);
  matrix->y0 = swfdec_bits_getsbits (bits, n_translate_bits);

  swfdec_matrix_ensure_invertible (matrix, inverse);
}

char *
swfdec_bits_get_string (SwfdecBits * bits)
{
  const char *s = swfdec_bits_skip_string (bits);

  return g_strdup (s);
}

const char *
swfdec_bits_skip_string (SwfdecBits *bits)
{
  char *s;
  const char *end;
  unsigned int len;

  swfdec_bits_syncbits (bits);
  end = memchr (bits->ptr, 0, bits->end - bits->ptr);
  if (end == NULL) {
    SWFDEC_ERROR ("could not parse string");
    return NULL;
  }
  
  len = end - (const char *) bits->ptr;
  s = (char *) bits->ptr;

  bits->ptr += len + 1;

  return s;
}

char *
swfdec_bits_get_string_length (SwfdecBits * bits, unsigned int len)
{
  char *ret;

  SWFDEC_BYTES_CHECK (bits, len);

  ret = g_strndup ((char *) bits->ptr, len);
  bits->ptr += len;
  return ret;
}

unsigned int
swfdec_bits_get_color (SwfdecBits * bits)
{
  int r, g, b;

  r = swfdec_bits_get_u8 (bits);
  g = swfdec_bits_get_u8 (bits);
  b = swfdec_bits_get_u8 (bits);

  return SWF_COLOR_COMBINE (r, g, b, 0xff);
}

unsigned int
swfdec_bits_get_rgba (SwfdecBits * bits)
{
  int r, g, b, a;

  r = swfdec_bits_get_u8 (bits);
  g = swfdec_bits_get_u8 (bits);
  b = swfdec_bits_get_u8 (bits);
  a = swfdec_bits_get_u8 (bits);

  return SWF_COLOR_COMBINE (r, g, b, a);
}

SwfdecGradient *
swfdec_bits_get_gradient (SwfdecBits * bits)
{
  SwfdecGradient *grad;
  unsigned int i, n_gradients;

  n_gradients = swfdec_bits_get_u8 (bits);
  grad = g_malloc (sizeof (SwfdecGradient) +
      sizeof (SwfdecGradientEntry) * (n_gradients - 1));
  grad->n_gradients = n_gradients;
  for (i = 0; i < n_gradients; i++) {
    grad->array[i].ratio = swfdec_bits_get_u8 (bits);
    grad->array[i].color = swfdec_bits_get_color (bits);
  }
  return grad;
}

SwfdecGradient *
swfdec_bits_get_gradient_rgba (SwfdecBits * bits)
{
  SwfdecGradient *grad;
  unsigned int i, n_gradients;

  n_gradients = swfdec_bits_get_u8 (bits);
  grad = g_malloc (sizeof (SwfdecGradient) +
      sizeof (SwfdecGradientEntry) * (n_gradients - 1));
  grad->n_gradients = n_gradients;
  for (i = 0; i < n_gradients; i++) {
    grad->array[i].ratio = swfdec_bits_get_u8 (bits);
    grad->array[i].color = swfdec_bits_get_rgba (bits);
  }
  return grad;
}

SwfdecGradient *
swfdec_bits_get_morph_gradient (SwfdecBits * bits)
{
  SwfdecGradient *grad;
  unsigned int i, n_gradients;

  n_gradients = swfdec_bits_get_u8 (bits);
  n_gradients *= 2;
  grad = g_malloc (sizeof (SwfdecGradient) +
      sizeof (SwfdecGradientEntry) * (n_gradients - 1));
  grad->n_gradients = n_gradients;
  for (i = 0; i < n_gradients; i++) {
    grad->array[i].ratio = swfdec_bits_get_u8 (bits);
    grad->array[i].color = swfdec_bits_get_rgba (bits);
  }
  return grad;
}

void
swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect)
{
  int nbits;
  
  swfdec_bits_syncbits (bits);
  nbits = swfdec_bits_getbits (bits, 5);

  rect->x0 = swfdec_bits_getsbits (bits, nbits);
  rect->x1 = swfdec_bits_getsbits (bits, nbits);
  rect->y0 = swfdec_bits_getsbits (bits, nbits);
  rect->y1 = swfdec_bits_getsbits (bits, nbits);
}

/**
 * swfdec_bits_get_buffer:
 * @bits: #SwfdecBits
 * @len: length of buffer or -1 for maximum
 *
 * Gets the contents of the next @len bytes of @bits and buts them in a new
 * subbuffer.
 *
 * Returns: the new #SwfdecBuffer
 **/
SwfdecBuffer *
swfdec_bits_get_buffer (SwfdecBits *bits, int len)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (len > 0 || len == -1, NULL);

  if (len > 0) {
    SWFDEC_BYTES_CHECK (bits, (unsigned int) len);
  } else {
    swfdec_bits_syncbits (bits);
    len = bits->end - bits->ptr;
    g_assert (len > 0);
  }
  buffer = swfdec_buffer_new_subbuffer (bits->buffer, bits->ptr - bits->buffer->data, len);
  bits->ptr += len;
  return buffer;
}
