/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_bits.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_rect.h"


#define SWFDEC_BITS_CHECK(b,n) G_STMT_START { \
  if (swfdec_bits_left(b) < (n)) { \
    SWFDEC_ERROR ("reading past end of buffer"); \
    b->ptr = b->end; \
    b->idx = 0; \
    return 0; \
  } \
}G_STMT_END
#define SWFDEC_BYTES_CHECK(b,n) G_STMT_START { \
  g_assert (b->end >= b->ptr); \
  g_assert (b->idx == 0); \
  if ((unsigned long) (b->end - b->ptr) < n) { \
    SWFDEC_ERROR ("reading past end of buffer"); \
    b->ptr = b->end; \
    b->idx = 0; \
    return 0; \
  } \
} G_STMT_END

/**
 * swfdec_bits_init:
 * @bits: a #SwfdecBits
 * @buffer: buffer to use for data or NULL
 *
 * initializes @bits for use with the data in @buffer. The buffer will not be
 * referenced, so you are responsible for keeping it around while @bits is used.
 **/
void 
swfdec_bits_init (SwfdecBits *bits, SwfdecBuffer *buffer)
{
  g_return_if_fail (bits != NULL);

  if (buffer) {
    bits->buffer = buffer;
    bits->ptr = buffer->data;
    bits->idx = 0;
    bits->end = buffer->data + buffer->length;
  } else {
    memset (bits, 0, sizeof (SwfdecBits));
  }
}

/**
 * swfdec_bits_init_bits:
 * @bits: a #SwfdecBits
 * @from: a #SwfdecBits to initialize from
 * @bytes: number of bytes to move to @bits
 *
 * Initializes @bits for use with the next @bytes bytes from @from. If not
 * enough bytes are available, less bytes will be available in @bits. @from
 * will skip the bytes now available in @bits. If you want to know if this
 * function moves enough bytes, you should ensure that enough data is 
 * available using swfdec_bits_left() before calling this function.
 **/
void
swfdec_bits_init_bits (SwfdecBits *bits, SwfdecBits *from, guint bytes)
{
  g_return_if_fail (bits != NULL);
  g_return_if_fail (from != NULL);
  g_return_if_fail (from->idx == 0);

  bits->buffer = from->buffer;
  bits->ptr = from->ptr;
  if (bytes > (guint) (from->end - from->ptr))
    bytes = from->end - from->ptr;
  bits->end = bits->ptr + bytes;
  bits->idx = 0;
  from->ptr = bits->end;
}

/**
 * swfdec_bits_init_data:
 * @bits: the #SwfdecBits to initialize
 * @data: data to initialize with
 * @len: length of the data
 *
 * Initializes @bits for use with the given @data. All operations on @bits will
 * return copies of the data, so after use, you can free the supplied data. Using 
 * %NULL for @data is valid if @len is 0.
 **/
void
swfdec_bits_init_data (SwfdecBits *bits, const guint8 *data, guint len)
{
  g_return_if_fail (bits != NULL);
  g_return_if_fail (data != NULL || len == 0);

  bits->buffer = NULL;
  bits->ptr = data;
  bits->idx = 0;
  bits->end = bits->ptr + len;
}

guint 
swfdec_bits_left (const SwfdecBits *b)
{
  if (b->ptr == NULL)
    return 0;
  g_assert (b->end >= b->ptr);
  g_assert (b->end > b->ptr || b->idx == 0);
  return (b->end - b->ptr) * 8 - b->idx;
}

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

guint
swfdec_bits_getbits (SwfdecBits * b, guint n)
{
  unsigned long r = 0;
  guint i;

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

guint
swfdec_bits_peekbits (const SwfdecBits * b, guint n)
{
  SwfdecBits tmp = *b;

  return swfdec_bits_getbits (&tmp, n);
}

int
swfdec_bits_getsbits (SwfdecBits * b, guint n)
{
  unsigned long r = 0;

  SWFDEC_BITS_CHECK (b, n);

  if (n == 0)
    return 0;
  r = -swfdec_bits_getbit (b);
  r = (r << (n - 1)) | swfdec_bits_getbits (b, n - 1);
  return r;
}

guint
swfdec_bits_peek_u8 (const SwfdecBits * b)
{
  g_assert (b->idx == 0);
  g_assert (b->ptr <= b->end);
  if (b->ptr == b->end)
    return 0;

  return *b->ptr;
}

guint
swfdec_bits_get_u8 (SwfdecBits * b)
{
  SWFDEC_BYTES_CHECK (b, 1);

  return *b->ptr++;
}

guint
swfdec_bits_get_u16 (SwfdecBits * b)
{
  guint r;

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

guint
swfdec_bits_get_u32 (SwfdecBits * b)
{
  guint r;

  SWFDEC_BYTES_CHECK (b, 4);

  r = b->ptr[0] | (b->ptr[1] << 8) | (b->ptr[2] << 16) | (b->ptr[3] << 24);
  b->ptr += 4;

  return r;
}

guint
swfdec_bits_get_bu16 (SwfdecBits *b)
{
  guint r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = (b->ptr[0] << 8) | b->ptr[1];
  b->ptr += 2;

  return r;
}

guint
swfdec_bits_get_bu24 (SwfdecBits *b)
{
  guint r;

  SWFDEC_BYTES_CHECK (b, 3);

  r = (b->ptr[0] << 16) | (b->ptr[1] << 8) | b->ptr[2];
  b->ptr += 3;

  return r;
}

guint 
swfdec_bits_get_bu32 (SwfdecBits *b)
{
  guint r;

  SWFDEC_BYTES_CHECK (b, 4);

  r = (b->ptr[0] << 24) | (b->ptr[1] << 16) | (b->ptr[2] << 8) | b->ptr[3];
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

  conv.i = (b->ptr[3] << 24) | (b->ptr[2] << 16) | (b->ptr[1] << 8) | b->ptr[0];
  b->ptr += 4;

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
double
swfdec_bits_get_double (SwfdecBits * b)
{
  union {
    guint32 i[2];
    double d;
  } conv;

  SWFDEC_BYTES_CHECK (b, 8);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  conv.i[1] = (b->ptr[3] << 24) | (b->ptr[2] << 16) | (b->ptr[1] << 8) | b->ptr[0];
  conv.i[0] = (b->ptr[7] << 24) | (b->ptr[6] << 16) | (b->ptr[5] << 8) | b->ptr[4];
#else
  conv.i[0] = (b->ptr[3] << 24) | (b->ptr[2] << 16) | (b->ptr[1] << 8) | b->ptr[0];
  conv.i[1] = (b->ptr[7] << 24) | (b->ptr[6] << 16) | (b->ptr[5] << 8) | b->ptr[4];
#if 0
  conv.i[0] = (b->ptr[0] << 24) | (b->ptr[1] << 16) | (b->ptr[2] << 8) | b->ptr[3];
  conv.i[1] = (b->ptr[4] << 24) | (b->ptr[5] << 16) | (b->ptr[6] << 8) | b->ptr[7];
#endif
#endif
  b->ptr += 8;

  return conv.d;
}

double
swfdec_bits_get_bdouble (SwfdecBits * b)
{
  guint8 x[8];
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  guint8 c;
#endif

  SWFDEC_BYTES_CHECK (b, 8);

  memcpy (&x, b->ptr, 8);
  b->ptr += 8;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  c=x[0]; x[0]=x[7]; x[7]=c;
  c=x[1]; x[1]=x[6]; x[6]=c;
  c=x[2]; x[2]=x[5]; x[5]=c;
  c=x[3]; x[3]=x[4]; x[4]=c;
#endif

  return *(double *)&x;
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

  ct->mask = FALSE;
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
  swfdec_bits_syncbits (bits);
}

void
swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix,
    cairo_matrix_t *inverse)
{
  int has_scale;
  int has_rotate;
  int n_translate_bits;

  has_scale = swfdec_bits_getbit (bits);
  if (has_scale) {
    int n_scale_bits = swfdec_bits_getbits (bits, 5);
    matrix->xx = SWFDEC_FIXED_TO_DOUBLE (swfdec_bits_getsbits (bits, n_scale_bits));
    matrix->yy = SWFDEC_FIXED_TO_DOUBLE (swfdec_bits_getsbits (bits, n_scale_bits));

    SWFDEC_LOG ("scalefactors: x = %d/65536, y = %d/65536", 
	SWFDEC_DOUBLE_TO_FIXED (matrix->xx), SWFDEC_DOUBLE_TO_FIXED (matrix->yy));
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

    SWFDEC_LOG ("skew: xy = %d/65536, yx = %d/65536", 
	SWFDEC_DOUBLE_TO_FIXED (matrix->xy), 
	SWFDEC_DOUBLE_TO_FIXED (matrix->yx));
  } else {
    SWFDEC_LOG ("no rotation");
    matrix->xy = 0;
    matrix->yx = 0;
  }
  n_translate_bits = swfdec_bits_getbits (bits, 5);
  matrix->x0 = swfdec_bits_getsbits (bits, n_translate_bits);
  matrix->y0 = swfdec_bits_getsbits (bits, n_translate_bits);

  swfdec_matrix_ensure_invertible (matrix, inverse);
  swfdec_bits_syncbits (bits);
}

static const char *
swfdec_bits_skip_string (SwfdecBits *bits)
{
  char *s;
  const char *end;
  guint len;

  SWFDEC_BYTES_CHECK (bits, 1);
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

/**
 * swfdec_bits_get_string:
 * @bits: a #SwfdecBits
 * @version: Flash player version
 *
 * Prior to Flash 6, strings used to be encoded as LATIN1. Since Flash 6, 
 * strings are encoded as UTF-8. This version does that check automatically
 * and converts strings to UTF-8.
 *
 * Returns: a UTF-8 encoded string or %NULL on error
 **/
char *
swfdec_bits_get_string (SwfdecBits *bits, guint version)
{
  const char *s;
  
  g_return_val_if_fail (bits != NULL, NULL);

  s = swfdec_bits_skip_string (bits);
  if (s == NULL)
    return NULL;

  if (version < 6) {
    char *ret = g_convert (s, -1, "UTF-8", "LATIN1", NULL , NULL, NULL);
    if (ret == NULL)
      g_warning ("Could not convert string from LATIN1 to UTF-8");
    return ret;
  } else {
    if (!g_utf8_validate (s, -1, NULL)) {
      SWFDEC_ERROR ("parsed string is not valid utf-8");
      return NULL;
    }
    return g_strdup (s);
  }
}

/**
 * swfdec_bits_skip_bytes:
 * @bits: a #SwfdecBits
 * @n_bytes: number of bytes to skip
 *
 * Skips up to @n_bytes bytes in @bits. If not enough bytes are available,
 * only the available amount is skipped and a warning is printed.
 *
 * Returns: the number of bytes actually skipped
 **/
guint
swfdec_bits_skip_bytes (SwfdecBits *bits, guint n_bytes)
{
  g_assert (bits->idx == 0);
  if ((guint) (bits->end - bits->ptr) < n_bytes) {
    SWFDEC_WARNING ("supposed to skip %u bytes, but only %td available",
	n_bytes, bits->end - bits->ptr);
    n_bytes = bits->end - bits->ptr;
  }
  bits->ptr += n_bytes;
  return n_bytes;
}

/**
 * swfdec_bits_get_string_length:
 * @bits: a #SwfdecBits
 * @len: number of bytes to read
 * @version: flash version number
 *
 * Reads the next @len bytes (not characters!) into a string and validates
 * its encoding is correct based on supplied version number.
 *
 * Returns: a new UTF-8 string or %NULL on error
 **/
char *
swfdec_bits_get_string_length (SwfdecBits * bits, guint len, guint version)
{
  char *ret;

  if (len == 0)
    return g_strdup ("");
  SWFDEC_BYTES_CHECK (bits, len);

  ret = g_strndup ((char *) bits->ptr, len);
  bits->ptr += len;

  if (version < 6) {
    char *tmp = g_convert (ret, -1, "UTF-8", "LATIN1", NULL , NULL, NULL);
    g_free(ret);
    ret = tmp;
  } else {
    if (!g_utf8_validate (ret, -1, NULL)) {
      SWFDEC_ERROR ("parsed string is not valid utf-8");
      g_free (ret);
      ret = NULL;
    }
  }

  return ret;
}

SwfdecColor
swfdec_bits_get_color (SwfdecBits * bits)
{
  guint r, g, b;

  r = swfdec_bits_get_u8 (bits);
  g = swfdec_bits_get_u8 (bits);
  b = swfdec_bits_get_u8 (bits);

  return SWFDEC_COLOR_COMBINE (r, g, b, 0xff);
}

SwfdecColor
swfdec_bits_get_rgba (SwfdecBits * bits)
{
  guint r, g, b, a;

  r = swfdec_bits_get_u8 (bits);
  g = swfdec_bits_get_u8 (bits);
  b = swfdec_bits_get_u8 (bits);
  a = swfdec_bits_get_u8 (bits);

  return SWFDEC_COLOR_COMBINE (r, g, b, a);
}

void
swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect)
{
  int nbits;
  
  nbits = swfdec_bits_getbits (bits, 5);
  rect->x0 = swfdec_bits_getsbits (bits, nbits);
  rect->x1 = swfdec_bits_getsbits (bits, nbits);
  rect->y0 = swfdec_bits_getsbits (bits, nbits);
  rect->y1 = swfdec_bits_getsbits (bits, nbits);

  swfdec_bits_syncbits (bits);
}

/**
 * swfdec_bits_get_buffer:
 * @bits: #SwfdecBits
 * @len: length of buffer or -1 for maximum
 *
 * Gets the contents of the next @len bytes of @bits and buts them in a new
 * subbuffer.
 *
 * Returns: the new #SwfdecBuffer or %NULL if the requested amount of data 
 *          isn't available
 **/
SwfdecBuffer *
swfdec_bits_get_buffer (SwfdecBits *bits, int len)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (len >= -1, NULL);

  if (len >= 0) {
    SWFDEC_BYTES_CHECK (bits, (guint) len);
  } else {
    g_assert (bits->idx == 0);
    len = bits->end - bits->ptr;
    g_assert (len >= 0);
  }
  if (len == 0)
    return swfdec_buffer_new (0);
  if (bits->buffer) {
    buffer = swfdec_buffer_new_subbuffer (bits->buffer, bits->ptr - bits->buffer->data, len);
  } else {
    buffer = swfdec_buffer_new (len);
    memcpy (buffer->data, bits->ptr, len);
  }
  bits->ptr += len;
  return buffer;
}

static void *
swfdec_bits_zalloc (void *opaque, guint items, guint size)
{
  return g_malloc (items * size);
}

static void
swfdec_bits_zfree (void *opaque, void *addr)
{
  g_free (addr);
}

/**
 * swfdec_bits_decompress:
 * @bits: a #SwfdecBits
 * @compressed: number of bytes to decompress or -1 for the rest
 * @decompressed: number of bytes to expect in the decompressed result or -1
 *                if unknown
 *
 * Decompresses the next @compressed bytes of data in @bits using the zlib
 * decompression algorithm and returns the result in a buffer. If @decompressed
 * was set and not enough data is available, the return buffer will be filled 
 * up with 0 bytes.
 *
 * Returns: a new #SwfdecBuffer containing the decompressed data or NULL on
 *          failure. If @decompressed &gt; 0, the buffer's length will be @decompressed.
 **/
SwfdecBuffer *
swfdec_bits_decompress (SwfdecBits *bits, int compressed, int decompressed)
{
  z_stream z = { 0, };
  SwfdecBuffer *buffer;
  int result;

  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (compressed >= -1, NULL);
  g_return_val_if_fail (decompressed > 0 || decompressed == -1, NULL);

  /* prepare the bits structure */
  if (compressed > 0) {
    SWFDEC_BYTES_CHECK (bits, (guint) compressed);
  } else {
    g_assert (bits->idx == 0);
    compressed = bits->end - bits->ptr;
    g_assert (compressed >= 0);
  }
  if (compressed == 0)
    return NULL;

  z.zalloc = swfdec_bits_zalloc;
  z.zfree = swfdec_bits_zfree;
  z.opaque = NULL;
  z.next_in = (Bytef *) bits->ptr;
  z.avail_in = compressed;
  result = inflateInit (&z);
  if (result != Z_OK) {
    SWFDEC_ERROR ("Error initialising zlib: %d %s", result, z.msg ? z.msg : "");
    goto fail;
  }
  buffer = swfdec_buffer_new (decompressed > 0 ? decompressed : compressed * 2);
  z.next_out = buffer->data;
  z.avail_out = buffer->length;
  while (TRUE) {
    result = inflate (&z, decompressed > 0 ? Z_FINISH : 0);
    switch (result) {
      case Z_STREAM_END:
	goto out;
      case Z_OK:
	if (decompressed < 0) {
	  buffer->data = g_realloc (buffer->data, buffer->length + compressed);
	  buffer->length += compressed;
	  z.next_out = buffer->data + z.total_out;
	  z.avail_out = buffer->length - z.total_out;
	  goto out;
	}
	/* else fall through */
      default:
	SWFDEC_ERROR ("error decompressing data: inflate returned %d %s",
	    result, z.msg ? z.msg : "");
	swfdec_buffer_unref (buffer);
	goto fail;
    }
  }
out:
  if (decompressed < 0) {
    buffer->length = z.total_out;
  } else {
    if (buffer->length < z.total_out) {
      SWFDEC_WARNING ("Not enough data decompressed: %lu instead of %"G_GSIZE_FORMAT" expected",
	  (gulong) z.total_out, buffer->length);
      memset (buffer->data + z.total_out, 0, buffer->length - z.total_out);
    }
  }
  result = inflateEnd (&z);
  if (result != Z_OK) {
    SWFDEC_ERROR ("error in inflateEnd: %d %s", result, z.msg ? z.msg : "");
  }
  bits->ptr += compressed;
  return buffer;

fail:
  bits->ptr += compressed;
  return NULL;
}
