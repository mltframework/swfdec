/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <math.h>

#include "swfdec_gradient_pattern.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_path.h"

G_DEFINE_TYPE (SwfdecGradientPattern, swfdec_gradient_pattern, SWFDEC_TYPE_PATTERN);

static void
swfdec_gradient_pattern_morph (SwfdecDraw *dest, SwfdecDraw *source, guint ratio)
{
  guint i;

  SwfdecGradientPattern *dpattern = SWFDEC_GRADIENT_PATTERN (dest);
  SwfdecGradientPattern *spattern = SWFDEC_GRADIENT_PATTERN (source);

  g_return_if_fail (spattern->end_gradient != NULL);
  dpattern->radial = spattern->radial;
  dpattern->focus = spattern->focus;
  dpattern->extend = spattern->extend;
  dpattern->n_gradients = spattern->n_gradients;
  for (i = 0; i < spattern->n_gradients; i++) {
    dpattern->gradient[i].color = swfdec_color_apply_morph (spattern->gradient[i].color,
	spattern->end_gradient[i].color, ratio);
    dpattern->gradient[i].ratio = (spattern->gradient[i].ratio * (65535 - ratio) +
	spattern->end_gradient[i].ratio * ratio) / 65535;
  }

  SWFDEC_DRAW_CLASS (swfdec_gradient_pattern_parent_class)->morph (dest, source, ratio);
}

static cairo_pattern_t *
swfdec_gradient_pattern_get_pattern (SwfdecPattern *pat, SwfdecRenderer *renderer,
    const SwfdecColorTransform *trans)
{
  guint i, ratio;
  cairo_pattern_t *pattern;
  SwfdecColor color;
  double offset;
  SwfdecGradientPattern *gradient = SWFDEC_GRADIENT_PATTERN (pat);

  if (gradient->n_gradients == 0) {
    /* cairo interprets an empty gradient as translucent, not as solid black */
    return cairo_pattern_create_rgb (0, 0, 0);
  }
#if 0
  /* use this when https://bugs.freedesktop.org/show_bug.cgi?id=8341 is fixed */
  if (gradient->radial)
    pattern = cairo_pattern_create_radial (0, 0, 0, 0, 0, 16384);
  else
    pattern = cairo_pattern_create_linear (-16384.0, 0, 16384.0, 0);
  cairo_pattern_set_matrix (pattern, &pat->transform);
#else
  {
    cairo_matrix_t mat = pat->transform;
    if (gradient->radial) {
      pattern = cairo_pattern_create_radial ((16384.0 / 256.0) * gradient->focus, 
	  0, 0, 0, 0, 16384 / 256.0);
    } else {
      pattern = cairo_pattern_create_linear (-16384.0 / 256.0, 0, 16384.0 / 256.0, 0);
    }
    cairo_matrix_scale (&mat, 1 / 256.0, 1 / 256.0);
    mat.x0 /= 256.0;
    mat.y0 /= 256.0;
    cairo_pattern_set_matrix (pattern, &mat);
  }
#endif
  cairo_pattern_set_extend (pattern, gradient->extend);
  /* we check here that ratios increase linearly, because both gradients parsed 
   * from the SWF and gradients created with beginGradientFill have this 
   * behavior */
  ratio = 0;
  for (i = 0; i < gradient->n_gradients; i++){
    color = swfdec_color_apply_transform (gradient->gradient[i].color,
	trans);
    ratio = MAX (ratio, gradient->gradient[i].ratio);
    offset = ratio / 255.0;
    cairo_pattern_add_color_stop_rgba (pattern, offset,
	SWFDEC_COLOR_RED (color) / 255.0, SWFDEC_COLOR_GREEN (color) / 255.0,
	SWFDEC_COLOR_BLUE (color) / 255.0, SWFDEC_COLOR_ALPHA (color) / 255.0);
    if (++ratio > 255)
      break;
  }
  return pattern;
}

static void
swfdec_gradient_pattern_class_init (SwfdecGradientPatternClass *klass)
{
  SWFDEC_DRAW_CLASS (klass)->morph = swfdec_gradient_pattern_morph;

  SWFDEC_PATTERN_CLASS (klass)->get_pattern = swfdec_gradient_pattern_get_pattern;
}

static void
swfdec_gradient_pattern_init (SwfdecGradientPattern *gradient)
{
  gradient->extend = CAIRO_EXTEND_PAD;
}

SwfdecDraw *
swfdec_gradient_pattern_new (void)
{
  return g_object_new (SWFDEC_TYPE_GRADIENT_PATTERN, NULL);
}

