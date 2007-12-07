/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_morph_movie.h"
#include "swfdec_debug.h"
#include "swfdec_draw.h"
#include "swfdec_player_internal.h"
#include "swfdec_stroke.h"

G_DEFINE_TYPE (SwfdecMorphMovie, swfdec_morph_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_morph_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  guint ratio = movie->original_ratio;
  SwfdecMorphMovie *mmovie = SWFDEC_MORPH_MOVIE (movie);
  SwfdecMorphShape *morph = mmovie->morph;
  SwfdecGraphic *graphic = SWFDEC_GRAPHIC (morph);
  extents->x0 = ((65535 - ratio) * graphic->extents.x0 + ratio * morph->end_extents.x0) / 65535;
  extents->x1 = ((65535 - ratio) * graphic->extents.x1 + ratio * morph->end_extents.x1) / 65535;
  extents->y0 = ((65535 - ratio) * graphic->extents.y0 + ratio * morph->end_extents.y0) / 65535;
  extents->y1 = ((65535 - ratio) * graphic->extents.y1 + ratio * morph->end_extents.y1) / 65535;
}

static void
swfdec_morph_movie_set_ratio (SwfdecMovie *movie)
{
  SwfdecMorphMovie *mmovie = SWFDEC_MORPH_MOVIE (movie);

  swfdec_movie_invalidate_next (movie);
  g_slist_foreach (mmovie->draws, (GFunc) g_object_unref, NULL);
  g_slist_free (mmovie->draws);
  mmovie->draws = NULL;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
}

static void
swfdec_morph_movie_create_morphs (SwfdecMorphMovie *mmovie)
{
  SwfdecShape *shape = SWFDEC_SHAPE (mmovie->morph);
  guint ratio = SWFDEC_MOVIE (mmovie)->original_ratio;
  GSList *walk;

  for (walk = shape->draws; walk; walk = walk->next) {
    mmovie->draws = g_slist_prepend (mmovie->draws, swfdec_draw_morph (walk->data, ratio));
  }
  mmovie->draws = g_slist_reverse (mmovie->draws);
}

static void
swfdec_morph_movie_render (SwfdecMovie *movie, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecMorphMovie *morph = SWFDEC_MORPH_MOVIE (movie);
  GSList *walk;

  if (morph->draws == NULL)
    swfdec_morph_movie_create_morphs (morph);

  for (walk = morph->draws; walk; walk = walk->next) {
    SwfdecDraw *draw = walk->data;

    if (!swfdec_rect_intersect (NULL, &draw->extents, inval))
      continue;
    
    swfdec_draw_paint (draw, cr, trans);
  }
}

static void
swfdec_morph_movie_invalidate (SwfdecMovie *movie, const cairo_matrix_t *matrix, gboolean last)
{
  SwfdecRect rect;
  
  swfdec_rect_transform (&rect, &movie->original_extents, matrix);
  swfdec_player_invalidate (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), &rect);
}

static void
swfdec_morph_movie_dispose (GObject *object)
{
  SwfdecMorphMovie *morph = SWFDEC_MORPH_MOVIE (object);

  g_slist_foreach (morph->draws, (GFunc) g_object_unref, NULL);
  g_slist_free (morph->draws);
  morph->draws = NULL;
  g_object_unref (morph->morph);

  G_OBJECT_CLASS (swfdec_morph_movie_parent_class)->dispose (object);
}

static void
swfdec_morph_movie_class_init (SwfdecMorphMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_morph_movie_dispose;

  movie_class->update_extents = swfdec_morph_movie_update_extents;
  movie_class->render = swfdec_morph_movie_render;
  movie_class->invalidate = swfdec_morph_movie_invalidate;
  movie_class->set_ratio = swfdec_morph_movie_set_ratio;
  /* FIXME */
  //movie_class->handle_mouse = swfdec_morph_movie_handle_mouse;
}

static void
swfdec_morph_movie_init (SwfdecMorphMovie *morph)
{
}

