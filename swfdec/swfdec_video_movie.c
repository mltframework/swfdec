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
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_sandbox.h"
#include "swfdec_utils.h"
#include "swfdec_video_provider.h"
#include "swfdec_video_video_provider.h"

G_DEFINE_TYPE (SwfdecVideoMovie, swfdec_video_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_video_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecVideo *org = SWFDEC_VIDEO (movie->graphic);
  SwfdecRect rect = { 0, 0, 
    SWFDEC_TWIPS_SCALE_FACTOR * org->width, 
    SWFDEC_TWIPS_SCALE_FACTOR * org->height };
  
  swfdec_rect_union (extents, extents, &rect);
}

static void
swfdec_video_movie_render (SwfdecMovie *mov, cairo_t *cr, 
    const SwfdecColorTransform *trans)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (mov);
  cairo_surface_t *surface;
  guint width, height;

  if (movie->provider == NULL || movie->clear)
    return;

  surface = swfdec_video_provider_get_image (movie->provider,
      swfdec_renderer_get (cr), &width, &height);
  if (surface == NULL)
    return;
  cairo_scale (cr, 
      (mov->original_extents.x1 - mov->original_extents.x0)
      / width,
      (mov->original_extents.y1 - mov->original_extents.y0)
      / height);
  cairo_set_source_surface (cr, surface, 0.0, 0.0);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
}

static void
swfdec_video_movie_new_image (SwfdecVideoProvider *provider, SwfdecVideoMovie *movie)
{
  movie->clear = FALSE;
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
}

static void
swfdec_video_movie_dispose (GObject *object)
{
  SwfdecVideoMovie *movie = SWFDEC_VIDEO_MOVIE (object);

  if (movie->provider) {
    g_signal_handlers_disconnect_by_func (movie->provider,
	swfdec_video_movie_new_image, movie);
    g_object_unref (movie->provider);
    movie->provider = NULL;
  }

  G_OBJECT_CLASS (swfdec_video_movie_parent_class)->dispose (object);
}

static void
swfdec_video_movie_set_ratio (SwfdecMovie *movie)
{
  SwfdecVideoMovie *video = SWFDEC_VIDEO_MOVIE (movie);

  if (video->provider)
    swfdec_video_provider_set_ratio (video->provider, movie->original_ratio);
}

static gboolean
swfdec_video_movie_get_variable (SwfdecAsObject *object, SwfdecAsObject *orig,
    const char *variable, SwfdecAsValue *val, guint *flags)
{
  guint version = swfdec_gc_object_get_context (object)->version;
  SwfdecVideoMovie *video;
  SwfdecAsContext *cx;

  video = SWFDEC_VIDEO_MOVIE (object);
  cx = swfdec_gc_object_get_context (video);

  if (swfdec_strcmp (version, variable, SWFDEC_AS_STR_width) == 0) {
    guint w;
    if (video->provider) {
      w = swfdec_video_provider_get_width (video->provider);
    } else {
      w = 0;
    }
    swfdec_as_value_set_integer (cx, val, w);
    return TRUE;
  } else if (swfdec_strcmp (version, variable, SWFDEC_AS_STR_height) == 0) {
    guint h;
    if (video->provider) {
      h = swfdec_video_provider_get_height (video->provider);
    } else {
      h = 0;
    }
    swfdec_as_value_set_integer (cx, val, h);
    return TRUE;
  } else if (swfdec_strcmp (version, variable, SWFDEC_AS_STR_deblocking) == 0) {
    SWFDEC_STUB ("Video.deblocking (get)");
    swfdec_as_value_set_number (cx, val, 0);
    return TRUE;
  } else if (swfdec_strcmp (version, variable, SWFDEC_AS_STR_smoothing) == 0) {
    SWFDEC_STUB ("Video.smoothing (get)");
    SWFDEC_AS_VALUE_SET_BOOLEAN (val, FALSE);
    return TRUE;
  } else {
    return SWFDEC_AS_OBJECT_CLASS (swfdec_video_movie_parent_class)->get (
	object, orig, variable, val, flags);
  }
}

static void
swfdec_video_movie_set_variable (SwfdecAsObject *object, const char *variable,
    const SwfdecAsValue *val, guint flags)
{
  guint version = swfdec_gc_object_get_context (object)->version;

  if (swfdec_strcmp (version, variable, SWFDEC_AS_STR_deblocking) == 0) {
    SWFDEC_STUB ("Video.deblocking (set)");
  } else if (swfdec_strcmp (version, variable, SWFDEC_AS_STR_smoothing) == 0) {
    SWFDEC_STUB ("Video.smoothing (set)");
  } else {
    SWFDEC_AS_OBJECT_CLASS (swfdec_video_movie_parent_class)->set (object,
	variable, val, flags);
  }
}

static gboolean
swfdec_video_movie_foreach_variable (SwfdecAsObject *object, SwfdecAsVariableForeach func, gpointer data)
{
  const char *native_variables[] = { SWFDEC_AS_STR_width, SWFDEC_AS_STR_height,
    SWFDEC_AS_STR_smoothing, SWFDEC_AS_STR_deblocking, NULL };
  int i;

  for (i = 0; native_variables[i] != NULL; i++) {
    SwfdecAsValue val;
    swfdec_as_object_get_variable (object, native_variables[i], &val);
    if (!func (object, native_variables[i], &val, 0, data))
      return FALSE;
  }

  return SWFDEC_AS_OBJECT_CLASS (swfdec_video_movie_parent_class)->foreach (
      object, func, data);
}

static void
swfdec_video_movie_invalidate (SwfdecMovie *movie, const cairo_matrix_t *matrix, gboolean last)
{
  SwfdecVideo *org = SWFDEC_VIDEO (movie->graphic);
  SwfdecRect rect = { 0, 0, 
    SWFDEC_TWIPS_SCALE_FACTOR * org->width, 
    SWFDEC_TWIPS_SCALE_FACTOR * org->height };

  swfdec_rect_transform (&rect, &rect, matrix);
  swfdec_player_invalidate (SWFDEC_PLAYER (swfdec_gc_object_get_context (movie)),
      movie, &rect);
}

static GObject *
swfdec_video_movie_constructor (GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties)
{
  GObject *object;
  SwfdecMovie *movie;
  SwfdecVideo *video;

  object = G_OBJECT_CLASS (swfdec_video_movie_parent_class)->constructor (type, 
      n_construct_properties, construct_properties);

  movie = SWFDEC_MOVIE (object);
  swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie), movie->resource->sandbox->Video);

  video = SWFDEC_VIDEO (movie->graphic);

  if (video->n_frames > 0) {
    SwfdecVideoProvider *provider = swfdec_video_video_provider_new (video);
    swfdec_video_movie_set_provider (SWFDEC_VIDEO_MOVIE (movie), provider);
    g_object_unref (provider);
  }

  return object;
}

static void
swfdec_video_movie_class_init (SwfdecVideoMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->constructor = swfdec_video_movie_constructor;
  object_class->dispose = swfdec_video_movie_dispose;

  asobject_class->get = swfdec_video_movie_get_variable;
  asobject_class->set = swfdec_video_movie_set_variable;
  asobject_class->foreach = swfdec_video_movie_foreach_variable;

  movie_class->update_extents = swfdec_video_movie_update_extents;
  movie_class->render = swfdec_video_movie_render;
  movie_class->invalidate = swfdec_video_movie_invalidate;
  movie_class->set_ratio = swfdec_video_movie_set_ratio;
}

static void
swfdec_video_movie_init (SwfdecVideoMovie * video_movie)
{
}

void
swfdec_video_movie_set_provider (SwfdecVideoMovie *movie, 
    SwfdecVideoProvider *provider)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_MOVIE (movie));
  g_return_if_fail (provider == NULL || SWFDEC_IS_VIDEO_PROVIDER (provider));

  if (provider == movie->provider)
    return;

  if (provider) {
    g_object_ref (provider);
    g_signal_connect (provider, "new-image", 
	G_CALLBACK (swfdec_video_movie_new_image), movie);
  }

  if (movie->provider) {
    g_signal_handlers_disconnect_by_func (movie->provider,
	swfdec_video_movie_new_image, movie);
    g_object_unref (movie->provider);
  }

  movie->provider = provider;
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
}

void
swfdec_video_movie_clear (SwfdecVideoMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_MOVIE (movie));

  movie->clear = TRUE;
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
}

