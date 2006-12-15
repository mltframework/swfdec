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

#ifndef __SWFDEC_BITS_H__
#define __SWFDEC_BITS_H__

#include <cairo.h>
#include <libswfdec/swfdec_color.h>
#include <libswfdec/swfdec_buffer.h>

#define SWFDEC_TEXT_SCALE_FACTOR		(1024.0)

typedef struct _SwfdecBits SwfdecBits;

struct _SwfdecBits
{
  SwfdecBuffer *buffer;
  unsigned char *ptr;
  unsigned int idx;
  unsigned char *end;
};

void swfdec_bits_init (SwfdecBits *bits, SwfdecBuffer *buffer);
unsigned int swfdec_bits_left (SwfdecBits *b);
int swfdec_bits_getbit (SwfdecBits * b);
unsigned int swfdec_bits_getbits (SwfdecBits * b, unsigned int n);
unsigned int swfdec_bits_peekbits (SwfdecBits * b, unsigned int n);
int swfdec_bits_getsbits (SwfdecBits * b, unsigned int n);
unsigned int swfdec_bits_peek_u8 (SwfdecBits * b);
unsigned int swfdec_bits_get_u8 (SwfdecBits * b);
unsigned int swfdec_bits_get_u16 (SwfdecBits * b);
int swfdec_bits_get_s16 (SwfdecBits * b);
unsigned int swfdec_bits_get_be_u16 (SwfdecBits * b);
unsigned int swfdec_bits_get_u32 (SwfdecBits * b);
float swfdec_bits_get_float (SwfdecBits * b);
double swfdec_bits_get_double (SwfdecBits * b);
void swfdec_bits_syncbits (SwfdecBits * b);

void swfdec_bits_get_color_transform (SwfdecBits * bits,
    SwfdecColorTransform * ct);
void swfdec_bits_get_transform (SwfdecBits * bits, SwfdecTransform *trans);
void swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix, 
    cairo_matrix_t *inverse);
const char *swfdec_bits_skip_string (SwfdecBits * bits);
char *swfdec_bits_get_string (SwfdecBits * bits);
char *swfdec_bits_get_string_length (SwfdecBits * bits, unsigned int len);
unsigned int swfdec_bits_get_color (SwfdecBits * bits);
unsigned int swfdec_bits_get_rgba (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_gradient (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_gradient_rgba (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_morph_gradient (SwfdecBits * bits);
void swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect);
SwfdecBuffer *swfdec_bits_get_buffer (SwfdecBits *bits, int len);


#endif
