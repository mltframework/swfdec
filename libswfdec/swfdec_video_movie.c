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

#include "swfdec_video_movie.h"

G_DEFINE_TYPE (SwfdecVideoMovie, swfdec_video_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_video_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecVideoMovie *video = SWFDEC_VIDEO_MOVIE (movie);
  SwfdecRect rect = { 0, 0, 
    SWFDEC_TWIPS_SCALE_FACTOR * video->video->width, 
    SWFDEC_TWIPS_SCALE_FACTOR * video->video->height };
  
  swfdec_rect_union (extents, extents, &rect);
}

static void
swfdec_video_movie_render (SwfdecMovie *mov, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (mov);
  cairo_surface_t *surface;

  if (movie->input == NULL)
    return;

  surface = movie->input->get_image (movie->input);
  cairo_scale (cr, SWFDEC_TWIPS_SCALE_FACTOR, SWFDEC_TWIPS_SCALE_FACTOR);
  if (surface != NULL) {
    cairo_set_source_surface (cr, surface, 0.0, 0.0);
    cairo_paint (cr);
  } else {
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_rectangle (cr, 0, 0, movie->video->width, movie->video->height);
    cairo_fill (cr);
  }
}

static void
swfdec_video_movie_unset_input (SwfdecVideoMovie *movie)
{
  if (movie->input == NULL)
    return;

  if (movie->input->finalize)
    movie->input->finalize (movie->input);
  movie->input = NULL;
}

static void
swfdec_video_movie_dispose (GObject *object)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (object);

  swfdec_video_movie_unset_input (movie);
  g_object_unref (movie->video);

  G_OBJECT_CLASS (swfdec_video_movie_parent_class)->dispose (object);
}

static gboolean
swfdec_video_movie_iterate_end (SwfdecMovie *mov)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (mov);

  if (!SWFDEC_MOVIE_CLASS (swfdec_video_movie_parent_class)->iterate_end (mov))
    return FALSE;

  if (movie->input && movie->input->iterate) {
    movie->input->iterate (movie->input);
  }

  return TRUE;
}

static void
swfdec_video_movie_class_init (SwfdecVideoMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_video_movie_dispose;

  movie_class->update_extents = swfdec_video_movie_update_extents;
  movie_class->render = swfdec_video_movie_render;
  movie_class->iterate_end = swfdec_video_movie_iterate_end;
}

static void
swfdec_video_movie_init (SwfdecVideoMovie * video_movie)
{
}

void
swfdec_video_movie_set_input (SwfdecVideoMovie *movie, SwfdecVideoMovieInput *input)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_MOVIE (movie));
  g_return_if_fail (input != NULL);
  g_return_if_fail (input->get_image);

  swfdec_video_movie_unset_input (movie);
  movie->input = input;
  input->movie = movie;
  swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
}

