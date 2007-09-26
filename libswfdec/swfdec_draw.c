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

/*** DRAW ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecDraw, swfdec_draw, G_TYPE_OBJECT);

static void
swfdec_draw_ensure_path_size (cairo_path_t *path, int elements)
{
#define STEPPING 32
  int current;

  current = path->num_data;
  current = current + STEPPING - 1;
  current -= current % STEPPING;
  elements = elements + STEPPING - 1;
  elements -= elements % STEPPING;
  if (elements <= current)
    return;

  path->data = g_realloc (path->data, elements);
#undef STEPPING
}

static void
swfdec_cairo_path_merge (cairo_path_t *dest, const cairo_path_t *start, const cairo_path_t *end, double ratio)
{
  int i;
  cairo_path_data_t *ddata, *sdata, *edata;
  double inv = 1.0 - ratio;

  g_assert (dest->num_data == start->num_data);
  g_assert (dest->num_data == end->num_data);

  ddata = dest->data;
  sdata = start->data;
  edata = end->data;
  for (i = 0; i < dest->num_data; i++) {
    g_assert (sdata[i].header.type == edata[i].header.type);
    ddata[i] = sdata[i];
    switch (sdata[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	ddata[i+1].point.x = sdata[i+1].point.x * inv + edata[i+1].point.x * ratio;
	ddata[i+1].point.y = sdata[i+1].point.y * inv + edata[i+1].point.y * ratio;
	ddata[i+2].point.x = sdata[i+2].point.x * inv + edata[i+2].point.x * ratio;
	ddata[i+2].point.y = sdata[i+2].point.y * inv + edata[i+2].point.y * ratio;
	i += 2;
      case CAIRO_PATH_MOVE_TO:
      case CAIRO_PATH_LINE_TO:
	ddata[i+1].point.x = sdata[i+1].point.x * inv + edata[i+1].point.x * ratio;
	ddata[i+1].point.y = sdata[i+1].point.y * inv + edata[i+1].point.y * ratio;
	i++;
      case CAIRO_PATH_CLOSE_PATH:
	break;
      default:
	g_assert_not_reached ();
    }
  }
}

static void
swfdec_draw_do_morph (SwfdecDraw* dest, SwfdecDraw *source, guint ratio)
{
  swfdec_draw_ensure_path_size (&dest->path, source->path.num_data);

  swfdec_cairo_path_merge (&dest->path, &source->path, &source->end_path, ratio / 65535.);
}

static void
swfdec_draw_dispose (GObject *object)
{
  SwfdecDraw *draw = SWFDEC_DRAW (object);

  g_free (draw->path.data);
  draw->path.data = NULL;
  draw->path.num_data = 0;
  g_free (draw->end_path.data);
  draw->end_path.data = NULL;
  draw->end_path.num_data = 0;

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

/**
 * swfdec_draw_append_path:
 * @draw: a #SwfdecDraw
 * @path: path to append to the drawing opeation
 * @end_path: end path of morph operation to append or %NULL if ot a morph 
 *            operation
 *
 * Appends the current path to the given drawing operation. Note that you need
 * to call swfdec_draw_recompute() before using this drawing operation for
 * drawing again.
 **/
void
swfdec_draw_append_path (SwfdecDraw *draw, const cairo_path_t *path,
    const cairo_path_t *end_path)
{
  g_return_if_fail (SWFDEC_IS_DRAW (draw));
  g_return_if_fail (path != NULL);
  g_return_if_fail (path->status == 0);
  g_return_if_fail (end_path == NULL || draw->path.num_data == draw->end_path.num_data);
  g_return_if_fail (end_path == NULL || path->num_data == end_path->num_data);
  

  swfdec_draw_ensure_path_size (&draw->path, draw->path.num_data + path->num_data);
  memcpy (&draw->path.data[draw->path.num_data], path->data, 
      sizeof (cairo_path_data_t) * path->num_data);
  draw->path.num_data += path->num_data;
  if (end_path) {
    swfdec_draw_ensure_path_size (&draw->end_path, draw->end_path.num_data + end_path->num_data);
    memcpy (&draw->end_path.data[draw->end_path.num_data], end_path->data, 
	sizeof (cairo_path_data_t) * end_path->num_data);
    draw->end_path.num_data += end_path->num_data;
  }
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
