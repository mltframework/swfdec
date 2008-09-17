/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_bitmap_pattern.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_renderer_internal.h"

enum {
  INVALIDATE,
  LAST_SIGNAL
};

G_DEFINE_TYPE (SwfdecBitmapPattern, swfdec_bitmap_pattern, SWFDEC_TYPE_PATTERN)
static guint signals[LAST_SIGNAL];

static cairo_pattern_t *
swfdec_bitmap_pattern_get_pattern (SwfdecPattern *pat, SwfdecRenderer *renderer,
    const SwfdecColorTransform *ctrans)
{
  SwfdecBitmapPattern *bitmap = SWFDEC_BITMAP_PATTERN (pat);
  cairo_pattern_t *pattern;

  pattern = swfdec_bitmap_data_get_pattern (bitmap->bitmap, renderer, ctrans);
  if (pattern == NULL)
    return NULL;
  cairo_pattern_set_matrix (pattern, &pat->transform);
  cairo_pattern_set_extend (pattern, bitmap->extend);
  cairo_pattern_set_filter (pattern, bitmap->filter);
  return pattern;
}

static void
swfdec_bitmap_pattern_morph (SwfdecDraw *dest, SwfdecDraw *source, guint ratio)
{
  SwfdecBitmapPattern *dpattern = SWFDEC_BITMAP_PATTERN (dest);
  SwfdecBitmapPattern *spattern = SWFDEC_BITMAP_PATTERN (source);

  dpattern->bitmap = g_object_ref (spattern->bitmap);
  dpattern->extend = spattern->extend;
  dpattern->filter = spattern->filter;

  SWFDEC_DRAW_CLASS (swfdec_bitmap_pattern_parent_class)->morph (dest, source, ratio);
}

static void
swfdec_bitmap_pattern_invalidate (SwfdecBitmapPattern *bitmap)
{
  g_signal_emit (bitmap, signals[INVALIDATE], 0);
}

static void
swfdec_bitmap_pattern_dispose (GObject *object)
{
  SwfdecBitmapPattern *bitmap = SWFDEC_BITMAP_PATTERN (object);

  g_signal_handlers_disconnect_by_func (bitmap->bitmap, 
      swfdec_bitmap_pattern_invalidate, bitmap);
  g_object_unref (bitmap->bitmap);

  G_OBJECT_CLASS (swfdec_bitmap_pattern_parent_class)->dispose (object);
}

static void
swfdec_bitmap_pattern_class_init (SwfdecBitmapPatternClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecDrawClass *draw_class = SWFDEC_DRAW_CLASS (klass);
  SwfdecPatternClass *pattern_class = SWFDEC_PATTERN_CLASS (klass);

  object_class->dispose = swfdec_bitmap_pattern_dispose;

  signals[INVALIDATE] = g_signal_new ("invalidate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  pattern_class->get_pattern = swfdec_bitmap_pattern_get_pattern;

  draw_class->morph = swfdec_bitmap_pattern_morph;
}

static void
swfdec_bitmap_pattern_init (SwfdecBitmapPattern *bitmap)
{
  bitmap->extend = CAIRO_EXTEND_REPEAT;
  bitmap->filter = CAIRO_FILTER_NEAREST;
}

SwfdecPattern *
swfdec_bitmap_pattern_new (SwfdecBitmapData *bitmap)
{
  SwfdecBitmapPattern *pattern;

  g_return_val_if_fail (SWFDEC_IS_BITMAP_DATA (bitmap), NULL);

  pattern = g_object_new (SWFDEC_TYPE_BITMAP_PATTERN, NULL);
  pattern->bitmap = bitmap;
  /* we ref the bitmap here as we are not garbage-collected, and we wanna keep a reference */
  g_object_ref (bitmap);
  g_signal_connect_swapped (pattern->bitmap, "invalidate", 
      G_CALLBACK (swfdec_bitmap_pattern_invalidate), pattern);

  return SWFDEC_PATTERN (pattern);
}
