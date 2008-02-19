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

#include "swfdec_filter.h"
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

cairo_pattern_t *
swfdec_filter_apply (SwfdecFilter *filter, cairo_pattern_t *pattern)
{
  SwfdecFilterClass *klass;
  cairo_pattern_t *ret;

  g_return_val_if_fail (SWFDEC_IS_FILTER (filter), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  klass = SWFDEC_FILTER_GET_CLASS (filter);
  g_assert (klass->apply);
  
  ret = klass->apply (filter, pattern);
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
	SWFDEC_WARNING ("    blur");
	swfdec_bits_skip_bytes (bits, 9);
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
	SWFDEC_WARNING ("    color matrix");
	swfdec_bits_skip_bytes (bits, 20 * 4);
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

