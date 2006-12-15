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

#ifndef __SWFDEC_COLOR_H__
#define __SWFDEC_COLOR_H__

#include <libswfdec/swfdec_types.h>

typedef unsigned int SwfdecColor;

/* NB: transforms may not be invertible like cairo_matrix_t */
struct _SwfdecTransform {
  SwfdecFixed	xx;			/* scale in x direction */
  SwfdecFixed	yy;			/* scale in y direction */
  SwfdecFixed	xy;			/* skew from y to x */
  SwfdecFixed	yx;			/* skew from x to y */
  SwfdecTwips	x0;			/* offset in x direction */
  SwfdecTwips	y0;			/* offset in y direction */
};

struct _SwfdecColorTransform {
  /* naming here is taken from ActionScript, where ?a is the multiplier and ?b the offset */
  int ra, rb, ga, gb, ba, bb, aa, ab;
};

typedef struct swfdec_gradient_struct SwfdecGradient;
typedef struct swfdec_gradient_entry_struct SwfdecGradientEntry;

struct swfdec_gradient_entry_struct
{
  int ratio;
  SwfdecColor color;
};

struct swfdec_gradient_struct
{
  unsigned int n_gradients;
  SwfdecGradientEntry array[1];
};

#define SWF_COLOR_COMBINE(r,g,b,a)	(((r)<<24) | ((g)<<16) | ((b)<<8) | (a))
#define SWF_COLOR_R(x)		(((x)>>24)&0xff)
#define SWF_COLOR_G(x)		(((x)>>16)&0xff)
#define SWF_COLOR_B(x)		(((x)>>8)&0xff)
#define SWF_COLOR_A(x)		((x)&0xff)

SwfdecColor swfdec_color_apply_morph (SwfdecColor start, SwfdecColor end, unsigned int ratio);
void swfdec_color_set_source (cairo_t *cr, SwfdecColor color);
void swfdec_color_transform_init_identity (SwfdecColorTransform * trans);
void swfdec_color_transform_init_color (SwfdecColorTransform *trans, SwfdecColor color);
void swfdec_color_transform_chain (SwfdecColorTransform *dest,
    const SwfdecColorTransform *last, const SwfdecColorTransform *first);
unsigned int swfdec_color_apply_transform (unsigned int in,
    const SwfdecColorTransform * trans);

void swfdec_transform_init_identity (SwfdecTransform *trans);
void swfdec_transform_to_matrix (cairo_matrix_t *matrix, cairo_matrix_t *inverse,
    const SwfdecTransform *trans);

#endif
