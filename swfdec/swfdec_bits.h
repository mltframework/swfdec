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

#ifndef __SWFDEC_BITS_H__
#define __SWFDEC_BITS_H__

#include <cairo.h>
#include <swfdec/swfdec_color.h>
#include <swfdec/swfdec_buffer.h>

G_BEGIN_DECLS

typedef struct _SwfdecBits SwfdecBits;

struct _SwfdecBits
{
  SwfdecBuffer *	buffer;		/* buffer data is taken from or NULL */
  const unsigned char *	ptr;		/* current location to read from */
  guint		idx;		/* bits already read from ptr */
  const unsigned char *	end;		/* pointer after last byte */
};

void swfdec_bits_init (SwfdecBits *bits, SwfdecBuffer *buffer);
void swfdec_bits_init_data (SwfdecBits *bits, const guint8 *data, guint len);
void swfdec_bits_init_bits (SwfdecBits *bits, SwfdecBits *from, guint bytes);
guint swfdec_bits_left (const SwfdecBits *b);
int swfdec_bits_getbit (SwfdecBits * b);
guint swfdec_bits_getbits (SwfdecBits * b, guint n);
guint swfdec_bits_peekbits (const SwfdecBits * b, guint n);
int swfdec_bits_getsbits (SwfdecBits * b, guint n);
guint swfdec_bits_peek_u8 (const SwfdecBits * b);
guint swfdec_bits_get_u8 (SwfdecBits * b);
guint swfdec_bits_get_u16 (SwfdecBits * b);
int swfdec_bits_get_s16 (SwfdecBits * b);
guint swfdec_bits_get_u32 (SwfdecBits * b);
guint swfdec_bits_get_bu16 (SwfdecBits *b);
guint swfdec_bits_get_bu24 (SwfdecBits *b);
guint swfdec_bits_get_bu32 (SwfdecBits *b);
float swfdec_bits_get_float (SwfdecBits * b);
double swfdec_bits_get_double (SwfdecBits * b);
double swfdec_bits_get_bdouble (SwfdecBits * b);
void swfdec_bits_syncbits (SwfdecBits * b);

void swfdec_bits_get_color_transform (SwfdecBits * bits,
    SwfdecColorTransform * ct);
void swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix, 
    cairo_matrix_t *inverse);
guint swfdec_bits_skip_bytes (SwfdecBits *bits, guint bytes);
char *swfdec_bits_get_string_length (SwfdecBits * bits, guint len, guint version);
char *swfdec_bits_get_string (SwfdecBits *bits, guint version);
SwfdecColor swfdec_bits_get_color (SwfdecBits * bits);
SwfdecColor swfdec_bits_get_rgba (SwfdecBits * bits);
void swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect);
SwfdecBuffer *swfdec_bits_get_buffer (SwfdecBits *bits, int len);
SwfdecBuffer *swfdec_bits_decompress (SwfdecBits *bits, int compressed, 
    int decompressed);


G_END_DECLS
#endif
