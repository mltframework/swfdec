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

#include "swfdec_draw.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_path.h"

/*** DRAW ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecDraw, swfdec_draw, G_TYPE_OBJECT);

static void
swfdec_draw_do_morph (SwfdecDraw* dest, SwfdecDraw *source, guint ratio)
{
  swfdec_path_merge (&dest->path, &source->path, &source->end_path, ratio / 65535.);
}

static void
swfdec_draw_dispose (GObject *object)
{
  SwfdecDraw *draw = SWFDEC_DRAW (object);

  swfdec_path_reset (&draw->path);
  swfdec_path_reset (&draw->end_path);

  G_OBJECT_CLASS (swfdec_draw_parent_class)->dispose (object);
}

static void
swfdec_draw_class_init (SwfdecDrawClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_draw_dispose;

  klass->morph = swfdec_draw_do_morph;
}

static void
swfdec_draw_init (SwfdecDraw *draw)
{
  swfdec_path_init (&draw->path);
  swfdec_path_init (&draw->end_path);
}

static gboolean
swfdec_draw_can_morph (SwfdecDraw *draw)
{
  return draw->end_path.num_data > 0;
}

/**
 * swfdec_draw_morph:
 * @draw: a #SwfdecDraw
 * @ratio: ratio of the morph from 0 to 65535
 *
 * Applies a morph to the given drawing operation. If the drawing operation
 * can not be morphed, it is returned unchanged. Otherwise a new drawing 
 * operation is created.
 *
 * Returns: a new reference to a #SwfdecDraw corresponding to the given morph 
 *          ratio
 **/
SwfdecDraw *
swfdec_draw_morph (SwfdecDraw *draw, guint ratio)
{
  SwfdecDrawClass *klass;
  SwfdecDraw *copy;

  g_return_val_if_fail (SWFDEC_IS_DRAW (draw), NULL);
  g_return_val_if_fail (ratio < 65536, NULL);

  if (!swfdec_draw_can_morph (draw) || ratio == 0) {
    /* not a morph */
    g_object_ref (draw);
    return draw;
  }
  klass = SWFDEC_DRAW_GET_CLASS (draw);
  g_assert (klass->morph);
  copy = g_object_new (G_OBJECT_CLASS_TYPE (klass), NULL);
  klass->morph (copy, draw, ratio);
  swfdec_draw_recompute (copy);
  return copy;
}

/**
 * swfdec_draw_paint:
 * @draw: drawing operation to perform
 * @cr: context to perform the operation on
 * @trans: color transofrmation to apply
 *
 * Draws the given drawing operation on the given Cairo context.
 **/
void
swfdec_draw_paint (SwfdecDraw *draw, cairo_t *cr, const SwfdecColorTransform *trans)
{
  SwfdecDrawClass *klass;

  g_return_if_fail (SWFDEC_IS_DRAW (draw));
  g_return_if_fail (draw->path.num_data > 0);
  g_return_if_fail (cr != NULL);
  g_return_if_fail (trans != NULL);

  klass = SWFDEC_DRAW_GET_CLASS (draw);
  g_assert (klass->paint);
  klass->paint (draw, cr, trans);
}

static gpointer
swfdec_draw_init_empty_surface (gpointer unused)
{
  return cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
}

/**
 * swfdec_draw_contains:
 * @draw: a #SwfdecDraw
 * @x: x coordinate to check
 * @y: y coordinate to check
 *
 * Checks if the given @x and @y coordinate is inside or outside the area drawn
 * by the given drawing operation.
 *
 * Returns: %TRUE if the coordinates are inside the drawing operation.
 **/
gboolean
swfdec_draw_contains (SwfdecDraw *draw, double x, double y)
{
  static GOnce empty_surface = G_ONCE_INIT;
  SwfdecDrawClass *klass;
  cairo_t *cr;
  gboolean result;
      
  g_return_val_if_fail (SWFDEC_IS_DRAW (draw), FALSE);

  if (!swfdec_rect_contains (&draw->extents, x, y))
    return FALSE;

  g_once (&empty_surface, swfdec_draw_init_empty_surface, NULL);

  klass = SWFDEC_DRAW_GET_CLASS (draw);
  g_assert (klass->contains);
  cr = cairo_create (empty_surface.retval);
  result = klass->contains (draw, cr, x, y);
  cairo_destroy (cr);
  return result;
}

/**
 * swfdec_draw_recompute:
 * @draw: a #SwfdecDraw
 *
 * This function must be called after a call to swfdec_draw_append_path() but 
 * before using swfdec_draw_paint() again. It updates internal state and caches,
 * in particular the extents of this drawing operation.
 **/
void
swfdec_draw_recompute (SwfdecDraw *draw)
{
  SwfdecDrawClass *klass;

  g_return_if_fail (SWFDEC_IS_DRAW (draw));

  klass = SWFDEC_DRAW_GET_CLASS (draw);
  g_assert (klass->compute_extents);
  klass->compute_extents (draw);
}
