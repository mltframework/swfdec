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

#ifndef __SWFDEC_COLOR_H__
#define __SWFDEC_COLOR_H__

#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

struct _SwfdecColorTransform {
  gboolean mask;			/* TRUE if this is a mask - masks are always black */
  /* naming here is taken from ActionScript, where ?a is the multiplier and ?b the offset */
  int ra, rb, ga, gb, ba, bb, aa, ab;
};

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define SWFDEC_COLOR_INDEX_ALPHA (3)
#define SWFDEC_COLOR_INDEX_RED (2)
#define SWFDEC_COLOR_INDEX_GREEN (1)
#define SWFDEC_COLOR_INDEX_BLUE (0)
#elif G_BYTE_ORDER == G_BIG_ENDIAN
#define SWFDEC_COLOR_INDEX_ALPHA (0)
#define SWFDEC_COLOR_INDEX_RED (1)
#define SWFDEC_COLOR_INDEX_GREEN (2)
#define SWFDEC_COLOR_INDEX_BLUE (3)
#else
#error "Unknown byte order"
#endif

#define SWFDEC_COLOR_COMBINE(r,g,b,a)	((SwfdecColor) (((a)<<24) | ((r)<<16) | ((g)<<8) | (b)))
#define SWFDEC_COLOR_OPAQUE(color)	((SwfdecColor) ((color) | SWFDEC_COLOR_COMBINE (0, 0, 0, 0xFF)))
#define SWFDEC_COLOR_ALPHA(x)		(((x)>>24)&0xff)
#define SWFDEC_COLOR_RED(x)		(((x)>>16)&0xff)
#define SWFDEC_COLOR_GREEN(x)		(((x)>>8)&0xff)
#define SWFDEC_COLOR_BLUE(x)		((x)&0xff)

#define SWFDEC_COLOR_WHITE		SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0xFF)

SwfdecColor swfdec_color_apply_morph (SwfdecColor start, SwfdecColor end, guint ratio);
void swfdec_color_set_source (cairo_t *cr, SwfdecColor color);
void swfdec_color_transform_init_identity (SwfdecColorTransform * trans);
void swfdec_color_transform_init_mask (SwfdecColorTransform * trans);
void swfdec_color_transform_init_color (SwfdecColorTransform *trans, SwfdecColor color);
gboolean swfdec_color_transform_is_identity (const SwfdecColorTransform * trans);
gboolean swfdec_color_transform_is_alpha (const SwfdecColorTransform * trans);
#define swfdec_color_transform_is_mask(trans) ((trans)->mask)
void swfdec_color_transform_chain (SwfdecColorTransform *dest,
    const SwfdecColorTransform *last, const SwfdecColorTransform *first);
SwfdecColor swfdec_color_apply_transform (SwfdecColor in,
    const SwfdecColorTransform * trans);
SwfdecColor swfdec_color_apply_transform_premultiplied (SwfdecColor in, 
    const SwfdecColorTransform * trans);

void swfdec_matrix_ensure_invertible (cairo_matrix_t *matrix, cairo_matrix_t *inverse);
double swfdec_matrix_get_xscale (const cairo_matrix_t *matrix);
double swfdec_matrix_get_yscale (const cairo_matrix_t *matrix);
double swfdec_matrix_get_rotation (const cairo_matrix_t *matrix);
void swfdec_matrix_morph (cairo_matrix_t *dest, const cairo_matrix_t *start,
    const cairo_matrix_t *end, guint ratio);


G_END_DECLS
#endif
