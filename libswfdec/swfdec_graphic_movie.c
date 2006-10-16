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

#include "swfdec_graphic_movie.h"

G_DEFINE_TYPE (SwfdecGraphicMovie, swfdec_graphic_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_graphic_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, 
      &SWFDEC_GRAPHIC_MOVIE (movie)->graphic->extents);
}

static void
swfdec_graphic_movie_render (SwfdecMovie *movie, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecGraphic *graphic = SWFDEC_GRAPHIC_MOVIE (movie)->graphic;

  swfdec_graphic_render (graphic, cr, trans, inval, fill);
}

static gboolean
swfdec_graphic_movie_handle_mouse (SwfdecMovie *movie, double x, double y,
    int button)
{
  SwfdecGraphic *graphic = SWFDEC_GRAPHIC_MOVIE (movie)->graphic;

  return swfdec_graphic_mouse_in (graphic, x, y);
}

static void
swfdec_graphic_movie_dispose (GObject *object)
{
  SwfdecGraphicMovie *movie = SWFDEC_GRAPHIC_MOVIE (object);

  g_object_unref (movie->graphic);

  G_OBJECT_CLASS (swfdec_graphic_movie_parent_class)->dispose (object);
}

static void
swfdec_graphic_movie_class_init (SwfdecGraphicMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_graphic_movie_dispose;

  movie_class->update_extents = swfdec_graphic_movie_update_extents;
  movie_class->render = swfdec_graphic_movie_render;
  movie_class->handle_mouse = swfdec_graphic_movie_handle_mouse;
}

static void
swfdec_graphic_movie_init (SwfdecGraphicMovie * graphic_movie)
{
}

