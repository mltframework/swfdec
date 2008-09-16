/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_filter.h"

#include "swfdec_blur_filter.h"
#include "swfdec_color_matrix_filter.h"
#include "swfdec_debug.h"

G_DEFINE_ABSTRACT_TYPE (SwfdecFilter, swfdec_filter, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_filter_class_init (SwfdecFilterClass *klass)
{
}

static void
swfdec_filter_init (SwfdecFilter *array)
{
}

SwfdecFilter *
swfdec_filter_clone (SwfdecFilter *filter)
{
  SwfdecFilter *clone;
  SwfdecFilterClass *klass;

  g_return_val_if_fail (SWFDEC_IS_FILTER (filter), NULL);

  klass = SWFDEC_FILTER_GET_CLASS (filter);
  clone = g_object_new (G_OBJECT_CLASS_TYPE (klass), "context", 
      swfdec_gc_object_get_context (filter), NULL);
  klass->clone (clone, filter);

  return clone;
}

cairo_pattern_t *
swfdec_filter_apply (SwfdecFilter *filter, cairo_pattern_t *pattern,
    double xscale, double yscale, const SwfdecRectangle *rect)
{
  SwfdecFilterClass *klass;
  cairo_pattern_t *ret;

  g_return_val_if_fail (SWFDEC_IS_FILTER (filter), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  klass = SWFDEC_FILTER_GET_CLASS (filter);
  g_assert (klass->apply);
  
  ret = klass->apply (filter, pattern, xscale, yscale, rect);
  cairo_pattern_destroy (pattern);
  return ret;
}

GSList *
swfdec_filter_parse (SwfdecPlayer *player, SwfdecBits *bits)
{
  GSList *filters = NULL;
  guint i, n_filters, filter_id;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (bits != NULL, NULL);

  n_filters = swfdec_bits_get_u8 (bits);
  SWFDEC_LOG ("  filters: %u", n_filters);
  for (i = 0; i < n_filters && swfdec_bits_left (bits); i++) {
    filter_id = swfdec_bits_get_u8 (bits);
    switch (filter_id) {
      case 0:
	SWFDEC_WARNING ("    drop shadow");
	swfdec_bits_skip_bytes (bits, 16);
	break;
      case 1:
	{
	  SwfdecBlurFilter *filter;
	  
	  filter = g_object_new (SWFDEC_TYPE_BLUR_FILTER, 
	      "context", player, NULL);
	  SWFDEC_LOG ("    blur");
	  filter->x = swfdec_bits_get_u32 (bits) / 65536.0;
	  filter->y = swfdec_bits_get_u32 (bits) / 65536.0;
	  filter->quality = swfdec_bits_getbits (bits, 5);
	  SWFDEC_LOG ("      x = %g", filter->x);
	  SWFDEC_LOG ("      y = %g", filter->x);
	  SWFDEC_LOG ("      quality = %u", filter->quality);
	  swfdec_bits_getbits (bits, 3);
	  filters = g_slist_prepend (filters, filter);
	}
	break;
      case 2:
	SWFDEC_WARNING ("    glow");
	swfdec_bits_skip_bytes (bits, 15);
	break;
      case 3:
	SWFDEC_WARNING ("    bevel");
	swfdec_bits_skip_bytes (bits, 27);
	break;
      case 4:
	{
	  guint n;
	  n = swfdec_bits_get_u8 (bits);
	  SWFDEC_WARNING ("    gradient glow");
	  swfdec_bits_skip_bytes (bits, n * 5 + 19);
	}
	break;
      case 5:
	{
	  guint x, y;
	  x = swfdec_bits_get_u8 (bits);
	  y = swfdec_bits_get_u8 (bits);
	  SWFDEC_WARNING ("    %u x %u convolution", x, y);
	  swfdec_bits_skip_bytes (bits, (x + y) * 4 + 13);
	}
	break;
      case 6:
	{
	  SwfdecColorMatrixFilter *filter;
	  guint j;
	  
	  filter = g_object_new (SWFDEC_TYPE_COLOR_MATRIX_FILTER, 
	      "context", player, NULL);
	  SWFDEC_LOG ("    color matrix");
	  for (j = 0; j < 20; j++) {
	    filter->matrix[j] = swfdec_bits_get_float (bits);
	  }
	  filters = g_slist_prepend (filters, filter);
	}
	break;
      case 7:
	{
	  guint n;
	  n = swfdec_bits_get_u8 (bits);
	  SWFDEC_WARNING ("    gradient bevel");
	  swfdec_bits_skip_bytes (bits, n * 5 + 19);
	}
	break;
      default:
	SWFDEC_ERROR ("unknown filter id %u", filter_id);
	break;
    }
  }

  filters = g_slist_reverse (filters);
  return filters;
}

void
swfdec_filter_skip (SwfdecBits *bits)
{
  guint i, n_filters, filter_id;

  g_return_if_fail (bits != NULL);

  n_filters = swfdec_bits_get_u8 (bits);
  for (i = 0; i < n_filters && swfdec_bits_left (bits); i++) {
    filter_id = swfdec_bits_get_u8 (bits);
    switch (filter_id) {
      case 0:
	swfdec_bits_skip_bytes (bits, 16);
	break;
      case 1:
	swfdec_bits_skip_bytes (bits, 9);
	break;
      case 2:
	swfdec_bits_skip_bytes (bits, 15);
	break;
      case 3:
	swfdec_bits_skip_bytes (bits, 27);
	break;
      case 4:
	{
	  guint n;
	  n = swfdec_bits_get_u8 (bits);
	  swfdec_bits_skip_bytes (bits, n * 5 + 19);
	}
	break;
      case 5:
	{
	  guint x, y;
	  x = swfdec_bits_get_u8 (bits);
	  y = swfdec_bits_get_u8 (bits);
	  swfdec_bits_skip_bytes (bits, (x + y) * 4 + 13);
	}
	break;
      case 6:
	swfdec_bits_skip_bytes (bits, 20 * 4);
	break;
      case 7:
	{
	  guint n;
	  n = swfdec_bits_get_u8 (bits);
	  swfdec_bits_skip_bytes (bits, n * 5 + 19);
	}
	break;
      default:
	SWFDEC_ERROR ("unknown filter id %u", filter_id);
	break;
    }
  }
}

void
swfdec_filter_get_rectangle (SwfdecFilter *filter, SwfdecRectangle *dest,
    double xscale, double yscale, const SwfdecRectangle *source)
{
  SwfdecFilterClass *klass;

  g_return_if_fail (SWFDEC_IS_FILTER (filter));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (source != NULL);

  klass = SWFDEC_FILTER_GET_CLASS (filter);
  klass->get_rectangle (filter, dest, xscale, yscale, source);
}
