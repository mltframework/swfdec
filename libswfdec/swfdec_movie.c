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
#include <string.h>
#include <math.h>

#include <js/jsapi.h>

#include "swfdec_movie.h"
#include "swfdec_debug.h"
#include "swfdec_event.h"
#include "swfdec_graphic.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*** MOVIE ***/

static const SwfdecContent default_content = {
  NULL, -1, 0, 0, { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
  { { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 } },
  NULL, NULL, NULL
};

G_DEFINE_ABSTRACT_TYPE (SwfdecMovie, swfdec_movie, G_TYPE_OBJECT)

static void
swfdec_movie_init (SwfdecMovie * movie)
{
  movie->content = &default_content;

  movie->xscale = 1.0;
  movie->yscale = 1.0;
  cairo_matrix_init_identity (&movie->transform);
  cairo_matrix_init_identity (&movie->inverse_transform);

  movie->visible = TRUE;
  movie->n_frames = 1;

  swfdec_rect_init_empty (&movie->extents);
}

/**
 * swfdec_movie_invalidate:
 * @movie: movie to invalidate
 *
 * Invalidates the area currently occupied by movie. If the area this movie
 * occupies has changed, call swfdec_movie_queue_update () instead.
 **/
void
swfdec_movie_invalidate (SwfdecMovie *movie)
{
  SwfdecRect rect = movie->extents;

  SWFDEC_LOG ("invalidating %g %g  %g %g", rect.x0, rect.y0, rect.x1, rect.y1);
  if (swfdec_rect_is_empty (&rect))
    return;
  while (movie->parent) {
    movie = movie->parent;
    if (movie->cache_state > SWFDEC_MOVIE_INVALID_EXTENTS)
      return;
    swfdec_rect_transform (&rect, &rect, &movie->transform);
  }
  swfdec_player_invalidate (SWFDEC_ROOT_MOVIE (movie)->player, &rect);
}

/**
 * swfdec_movie_queue_update:
 * @movie: a #SwfdecMovie
 * @state: how much needs to be updated
 *
 * Queues an update of all cached values inside @movie.
 **/
void
swfdec_movie_queue_update (SwfdecMovie *movie, SwfdecMovieState state)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  while (movie && movie->cache_state < state) {
    movie->cache_state = state;
    movie = movie->parent;
    state = SWFDEC_MOVIE_INVALID_CHILDREN;
  }
}

static void
swfdec_movie_update_extents (SwfdecMovie *movie)
{
  SwfdecMovieClass *klass;
  GList *walk;
  SwfdecRect *rect = &movie->original_extents;
  SwfdecRect *extents = &movie->extents;

  swfdec_rect_init_empty (rect);
  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_rect_union (rect, rect, &SWFDEC_MOVIE (walk->data)->extents);
  }
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->update_extents)
    klass->update_extents (movie, rect);
  if (swfdec_rect_is_empty (rect)) {
    *extents = *rect;
    return;
  }
  swfdec_rect_transform (extents, rect, &movie->transform);
  swfdec_rect_transform (rect, rect, &movie->content->transform);
  if (movie->parent && movie->parent->cache_state < SWFDEC_MOVIE_INVALID_EXTENTS) {
    SwfdecRect tmp;
    swfdec_rect_transform (&tmp, extents, &movie->parent->transform);
    if (!swfdec_rect_inside (&SWFDEC_MOVIE (movie->parent)->extents, &tmp))
      swfdec_movie_queue_update (movie->parent, SWFDEC_MOVIE_INVALID_EXTENTS);
  }
}

static void
swfdec_movie_update_matrix (SwfdecMovie *movie)
{
  cairo_matrix_t *mat = &movie->transform;

  cairo_matrix_init_translate (mat, movie->x, movie->y);
  cairo_matrix_scale (mat, movie->xscale, movie->yscale);
  cairo_matrix_rotate (mat, movie->rotation * M_PI / 180);
  cairo_matrix_multiply (mat, mat, &movie->content->transform);
  movie->inverse_transform = *mat;
  if (cairo_matrix_invert (&movie->inverse_transform)) {
    /* the matrix is somehow weird, carefully adjust it */
    mat = &movie->inverse_transform;
    *mat = movie->content->transform;
    if (cairo_matrix_invert (mat)) {
      g_assert_not_reached ();
    }
    cairo_matrix_rotate (mat, -movie->rotation * M_PI / 180);
    if (movie->xscale == 0) {
      cairo_matrix_scale (mat, G_MAXDOUBLE, 1);
    } else {
      cairo_matrix_scale (mat, 1 / movie->xscale, 1);
    }
    if (movie->yscale == 0) {
      cairo_matrix_scale (mat, 1, G_MAXDOUBLE);
    } else {
      cairo_matrix_scale (mat, 1, 1 / movie->yscale);
    }
    cairo_matrix_translate (mat, -movie->x, -movie->y);
  }
  swfdec_movie_update_extents (movie);
}

static void
swfdec_movie_do_update (SwfdecMovie *movie)
{
  GList *walk;

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *child = walk->data;

    if (child->cache_state != SWFDEC_MOVIE_UP_TO_DATE)
      swfdec_movie_do_update (child);
  }

  switch (movie->cache_state) {
    case SWFDEC_MOVIE_INVALID_CHILDREN:
      break;
    case SWFDEC_MOVIE_INVALID_EXTENTS:
      swfdec_movie_update_extents (movie);
      break;
    case SWFDEC_MOVIE_INVALID_MATRIX:
      swfdec_movie_update_matrix (movie);
      break;
    case SWFDEC_MOVIE_UP_TO_DATE:
    default:
      g_assert_not_reached ();
  }
  if (movie->cache_state > SWFDEC_MOVIE_INVALID_EXTENTS)
    swfdec_movie_invalidate (movie);
  movie->cache_state = SWFDEC_MOVIE_UP_TO_DATE;
}

/**
 * swfdec_movie_update:
 * @movie: a #SwfdecMovie
 *
 * Brings the cached values of @movie up-to-date if they are not. This includes
 * transformation matrices and extents. It needs to be called before accessing
 * the relevant values.
 **/
void
swfdec_movie_update (SwfdecMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (movie->cache_state == SWFDEC_MOVIE_UP_TO_DATE)
    return;

  if (movie->parent && movie->parent->cache_state != SWFDEC_MOVIE_UP_TO_DATE) {
    swfdec_movie_update (movie->parent);
  } else {
    swfdec_movie_do_update (movie);
  }
}

/**
 * swfdec_movie_set_content:
 * @movie: a #SwfdecMovie
 * @content: #SwfdecContent to set for this movie or NULL to unset
 *
 * Sets new contents for @movie. Note that name and displayed object must be
 * identical to the previous contents.
 **/
void
swfdec_movie_set_content (SwfdecMovie *movie, const SwfdecContent *content)
{
  const SwfdecContent *old_content;
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (content == NULL) {
    content = &default_content;
    if (movie->content->name && movie->jsobj)
      swfdec_js_movie_remove_property (movie);
  } else if (movie->content != &default_content) {
    g_return_if_fail (movie->content->depth == content->depth);
    g_return_if_fail (movie->content->graphic == content->graphic);
    if (content->name) {
      g_return_if_fail (movie->content->name != NULL);
      g_return_if_fail (g_str_equal (content->name, movie->content->name));
    } else {
      g_return_if_fail (movie->content->name == NULL);
    }
  }
  old_content = movie->content;
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->content_changed)
    klass->content_changed (movie, content);
  movie->content = content;
  if (old_content == &default_content && content->name && movie->parent && movie->parent->jsobj)
    swfdec_js_movie_add_property (movie);

  swfdec_movie_invalidate (movie);
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

/**
 * swfdec_movie_execute:
 * @movie: a #SwfdecMovie
 * @condition: the event that should happen
 *
 * Executes all scripts associated with the given event.
 **/
void
swfdec_movie_execute (SwfdecMovie *movie, SwfdecEventType condition)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (condition != 0);

  if (movie->content->events == NULL)
    return;
  swfdec_event_list_execute (movie->content->events, movie, condition, 0);
}

/**
 * swfdec_movie_remove:
 * @movie: #SwfdecMovie to remove
 *
 * Removes this movie from its parent. After this it will no longer be present.
 **/
void
swfdec_movie_remove (SwfdecMovie *movie)
{
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  /* remove all children */
  while (movie->list) {
    swfdec_movie_remove (movie->list->data);
  }
  swfdec_movie_execute (movie, SWFDEC_EVENT_UNLOAD);
  swfdec_movie_set_content (movie, NULL);
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  swfdec_movie_invalidate (movie);
  if (movie->parent) {
    if (klass->set_parent)
      klass->set_parent (movie, NULL);
    if (klass->iterate_start || klass->iterate_end) {
      SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (movie->root)->player;
      player->movies = g_list_remove (player->movies, movie);
    }
    movie->parent->list = g_list_remove (movie->parent->list, movie);
    movie->parent = NULL;
  }
  g_object_unref (movie);
}

gboolean
swfdec_movie_handle_mouse (SwfdecMovie *movie,
      double x, double y, int button)
{
  GList *g;
  unsigned int clip_depth = 0;
  SwfdecMovieClass *klass;

  g_assert (button == 0 || button == 1);

  cairo_matrix_transform_point (&movie->inverse_transform, &x, &y);
  movie->mouse_in = FALSE;
  movie->mouse_x = x;
  movie->mouse_y = y;
  SWFDEC_LOG ("moviclip %p mouse: %g %g", movie, x, y);
  movie->mouse_button = button;
  g = movie->list;
  while (g) {
    SwfdecMovie *child = g->data;
    g = g->next;
    if (child->content->clip_depth) {
      /* FIXME: we should probably create a cairo_t here, render the clip
       * area into it and check if the mouse hits it. If it does,
       * include the clipped movies, otherwise skip them.
       */
      SWFDEC_LOG ("clip_depth=%d", child->content->clip_depth);
      clip_depth = child->content->clip_depth;
    }

    if (clip_depth && child->content->depth <= clip_depth) {
      SWFDEC_DEBUG ("clipping depth=%d", child->content->clip_depth);
      continue;
    }

    if (swfdec_movie_handle_mouse (child, x, y, button))
      movie->mouse_in = TRUE;
  }
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->handle_mouse)
    movie->mouse_in = klass->handle_mouse (movie, x, y, button);
  return movie->mouse_in;
}

void
swfdec_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *color_transform, const SwfdecRect *inval, gboolean fill)
{
  SwfdecMovieClass *klass;
  GList *g;
  unsigned int clip_depth = 0;
  SwfdecColorTransform trans;
  SwfdecRect rect;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (cr != NULL);
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
    g_warning ("%s", cairo_status_to_string (cairo_status (cr)));
  }
  g_return_if_fail (color_transform != NULL);
  g_return_if_fail (inval != NULL);
  
  if (!swfdec_rect_intersect (NULL, &movie->extents, inval)) {
    SWFDEC_LOG ("not rendering %s %p, extents %g %g  %g %g are not in invalid area %g %g  %g %g",
	G_OBJECT_TYPE_NAME (movie), movie, 
	movie->extents.x0, movie->extents.y0, movie->extents.x1, movie->extents.y1,
	inval->x0, inval->y0, inval->x1, inval->y1);
    return;
  }

  cairo_save (cr);

  SWFDEC_LOG ("transforming movie, transform: %g %g  %g %g   %g %g",
      movie->transform.xx, movie->transform.yy,
      movie->transform.xy, movie->transform.yx,
      movie->transform.x0, movie->transform.y0);
  cairo_transform (cr, &movie->transform);
  swfdec_rect_transform (&rect, inval, &movie->inverse_transform);
  SWFDEC_LOG ("%sinvalid area is now: %g %g  %g %g",  movie->parent ? "  " : "",
      rect.x0, rect.y0, rect.x1, rect.y1);
  swfdec_color_transform_chain (&trans, &movie->content->color_transform, color_transform);

  for (g = movie->list; g; g = g_list_next (g)) {
    SwfdecMovie *child = g->data;

    if (child->content->clip_depth) {
      if (clip_depth) {
	/* FIXME: is clipping additive? */
	SWFDEC_INFO ("unsetting clip depth %d for new clip depth", clip_depth);
	cairo_restore (cr);
	clip_depth = 0;
      }
      if (fill == FALSE) {
	SWFDEC_WARNING ("clipping inside clipping not implemented");
      } else {
	SWFDEC_INFO ("clipping up to depth %d by using %p with depth %d", child->content->clip_depth,
	    child, child->content->depth);
	clip_depth = child->content->clip_depth;
	cairo_save (cr);
	swfdec_movie_render (child, cr, &trans, &rect, FALSE);
	cairo_clip (cr);
	continue;
      }
    }

    if (clip_depth && child->content->depth > clip_depth) {
      SWFDEC_INFO ("unsetting clip depth %d for depth %d", clip_depth, child->content->depth);
      clip_depth = 0;
      cairo_restore (cr);
    }

    SWFDEC_LOG ("rendering %p with depth %d", child, child->content->depth);
    swfdec_movie_render (child, cr, &trans, &rect, fill);
  }
  if (clip_depth) {
    SWFDEC_INFO ("unsetting clip depth %d after rendering", clip_depth);
    clip_depth = 0;
    cairo_restore (cr);
  }
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->render)
    klass->render (movie, cr, &trans, &rect, fill);
#if 0
  /* code to draw a red rectangle around the area occupied by this movie clip */
  {
    double x = 1.0, y = 0.0;
    cairo_transform (cr, &movie->inverse_transform);
    cairo_user_to_device_distance (cr, &x, &y);
    cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
    cairo_set_line_width (cr, 1 / sqrt (x * x + y * y));
    cairo_rectangle (cr, object->extents.x0 + 10, object->extents.y0 + 10,
	object->extents.x1 - object->extents.x0 - 20,
	object->extents.y1 - object->extents.y0 - 20);
    cairo_stroke (cr);
  }
#endif
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
    g_warning ("%s", cairo_status_to_string (cairo_status (cr)));
  }
  cairo_restore (cr);
}

static void
swfdec_movie_dispose (GObject *object)
{
  SwfdecMovie * movie = SWFDEC_MOVIE (object);

  g_assert (movie->jsobj == NULL);
  g_assert (movie->list == NULL);
  g_assert (movie->content == &default_content);

  G_OBJECT_CLASS (swfdec_movie_parent_class)->dispose (G_OBJECT (movie));
}

static void
swfdec_movie_class_init (SwfdecMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_movie_dispose;
}

void
swfdec_movie_set_parent (SwfdecMovie *movie, SwfdecMovie *parent)
{
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (movie->parent == NULL);
  g_return_if_fail (SWFDEC_IS_MOVIE (parent));

  movie->parent = parent;
  movie->root = parent->root;
  parent->list = g_list_insert_sorted (parent->list, movie, swfdec_movie_compare_depths);
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  /* NB: adding to the movies list happens before setting the parent.
   * Setting the parent does a gotoAndPlay(0) for Sprites which can cause
   * new movies to be created (and added to this list)
   */
  if (klass->iterate_start || klass->iterate_end) {
    SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (movie->root)->player;
    player->movies = g_list_prepend (player->movies, movie);
  }
  if (klass->set_parent)
    klass->set_parent (movie, parent);
  SWFDEC_DEBUG ("inserting %s %p (depth %u) into %s %p", G_OBJECT_TYPE_NAME (movie), movie,
      movie->content->depth,  G_OBJECT_TYPE_NAME (parent), parent);
  swfdec_movie_execute (movie, SWFDEC_EVENT_LOAD);
}

/**
 * swfdec_movie_new:
 * @parent: the parent movie that will contain this movie
 * @content: the content to display
 *
 * Creates a new #SwfdecMovie as a child of @parent with the given content.
 * @parent must not contain a movie clip at the depth specified by @content.
 *
 * Returns: a new #SwfdecMovie
 **/
SwfdecMovie *
swfdec_movie_new (SwfdecMovie *parent, const SwfdecContent *content)
{
  SwfdecGraphicClass *klass;
  SwfdecMovie *ret;
  const SwfdecContent *old;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (parent), NULL);
  g_return_val_if_fail (SWFDEC_IS_GRAPHIC (content->graphic), NULL);

  SWFDEC_DEBUG ("new movie for parent %p", parent);
  klass = SWFDEC_GRAPHIC_GET_CLASS (content->graphic);
  g_return_val_if_fail (klass->create_movie != NULL, NULL);
  ret = klass->create_movie (content->graphic);
  old = ret->content;
  ret->content = content;
  swfdec_movie_set_parent (ret, parent);
  ret->content = old;
  swfdec_movie_set_content (ret, content);

  return ret;
}

void
swfdec_movie_goto (SwfdecMovie *movie, guint frame)
{
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (frame < movie->n_frames);

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->goto_frame)
    klass->goto_frame (movie, frame);
}

int
swfdec_movie_compare_depths (gconstpointer a, gconstpointer b)
{
  return (int) (SWFDEC_MOVIE (a)->content->depth - SWFDEC_MOVIE (b)->content->depth);
}

