/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "swfdec_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_button_movie.h"
#include "swfdec_debug.h"
#include "swfdec_draw.h"
#include "swfdec_event.h"
#include "swfdec_filter.h"
#include "swfdec_graphic.h"
#include "swfdec_image.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sandbox.h"
#include "swfdec_system.h"
#include "swfdec_text_field_movie.h"
#include "swfdec_utils.h"
#include "swfdec_video_movie.h"

/*** MOVIE ***/

enum {
  PROP_0,
  PROP_DEPTH,
  PROP_GRAPHIC,
  PROP_NAME,
  PROP_PARENT,
  PROP_RESOURCE
};

enum {
  MATRIX_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_ABSTRACT_TYPE (SwfdecMovie, swfdec_movie, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_movie_init (SwfdecMovie * movie)
{
  movie->blend_mode = SWFDEC_BLEND_MODE_NORMAL;

  movie->xscale = 100;
  movie->yscale = 100;
  cairo_matrix_init_identity (&movie->original_transform);
  cairo_matrix_init_identity (&movie->matrix);
  cairo_matrix_init_identity (&movie->inverse_matrix);

  swfdec_color_transform_init_identity (&movie->color_transform);

  movie->visible = TRUE;
  movie->cache_state = SWFDEC_MOVIE_INVALID_EXTENTS;
  movie->invalidate_last = TRUE;
  movie->invalidate_next = TRUE;

  swfdec_rect_init_empty (&movie->extents);
}

/**
 * swfdec_movie_invalidate:
 * @movie: a #SwfdecMovie
 * @parent_to_global: This is the matrix from the parent to the global matrix.
 *                    It is only used for caching reasons
 * @new_contents: %TRUE if this is the invalidation of the new contents, %FALSE 
 *                if the old contents are invalidated.
 *
 * Performs an instant invalidation on @movie. You most likely don't want to
 * call this function directly, but use swfdec_movie_invalidate_last() or
 * swfdec_movie_invalidate_next() instead.
 **/
void
swfdec_movie_invalidate (SwfdecMovie *movie, const cairo_matrix_t *parent_to_global,
    gboolean new_contents)
{
  SwfdecMovieClass *klass;
  cairo_matrix_t matrix;

  if (new_contents) {
    movie->invalidate_next = FALSE;
  } else {
    SwfdecPlayer *player;
    if (movie->invalidate_last)
      return;
    movie->invalidate_last = TRUE;
    player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
    player->priv->invalid_pending = g_slist_prepend (player->priv->invalid_pending, movie);
  }
  g_assert (movie->cache_state <= SWFDEC_MOVIE_INVALID_CHILDREN);
  SWFDEC_LOG ("invalidating %s %s at %s", G_OBJECT_TYPE_NAME (movie), 
      movie->name, new_contents ? "end" : "start");
  cairo_matrix_multiply (&matrix, &movie->matrix, parent_to_global);
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  klass->invalidate (movie, &matrix, new_contents);
}

/**
 * swfdec_movie_invalidate_last:
 * @movie: a #SwfdecMovie
 *
 * Ensures the movie's contents are invalidated. This function must be called
 * before changing the movie or the output will have artifacts.
 **/
void
swfdec_movie_invalidate_last (SwfdecMovie *movie)
{
  cairo_matrix_t matrix;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (movie->invalidate_last)
    return;

  if (movie->parent)
    swfdec_movie_local_to_global_matrix (movie->parent, &matrix);
  else
    cairo_matrix_init_identity (&matrix);
  swfdec_movie_invalidate (movie, &matrix, FALSE);
  g_assert (movie->invalidate_last);
}

/**
 * swfdec_movie_invalidate_next:
 * @movie: a #SwfdecMovie
 *
 * Ensures the movie will be invalidated after script execution is done. So
 * after calling this function you can modify position and contents of the 
 * @movie in any way.
 **/
void
swfdec_movie_invalidate_next (SwfdecMovie *movie)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  swfdec_movie_invalidate_last (movie);
  movie->invalidate_next = TRUE;
  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  if (movie == SWFDEC_MOVIE (player->priv->focus))
    swfdec_player_invalidate_focusrect (player);
}

/**
 * swfdec_movie_queue_update:
 * @movie: a #SwfdecMovie
 * @state: how much needs to be updated
 *
 * Queues an update of all cached values inside @movie and invalidates it.
 **/
void
swfdec_movie_queue_update (SwfdecMovie *movie, SwfdecMovieCacheState state)
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

  *rect = movie->draw_extents;
  if (movie->image) {
    SwfdecRect image_extents = { 0, 0, 
      movie->image->width * SWFDEC_TWIPS_SCALE_FACTOR,
      movie->image->height * SWFDEC_TWIPS_SCALE_FACTOR };
    swfdec_rect_union (rect, rect, &image_extents);
  }
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
  swfdec_rect_transform (extents, rect, &movie->matrix);
  if (movie->parent && movie->parent->cache_state < SWFDEC_MOVIE_INVALID_EXTENTS) {
    /* no need to invalidate here */
    movie->parent->cache_state = SWFDEC_MOVIE_INVALID_EXTENTS;
  }
}

void
swfdec_movie_begin_update_matrix (SwfdecMovie *movie)
{
  swfdec_movie_invalidate_next (movie);
}

void
swfdec_movie_end_update_matrix (SwfdecMovie *movie)
{
  double d, e;

  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);

  /* we operate on x0 and y0 when setting movie._x and movie._y */
  if (movie->modified) {
    movie->matrix.xx = movie->original_transform.xx;
    movie->matrix.yx = movie->original_transform.yx;
    movie->matrix.xy = movie->original_transform.xy;
    movie->matrix.yy = movie->original_transform.yy;
  } else {
    movie->matrix = movie->original_transform;
  }

  d = movie->xscale / swfdec_matrix_get_xscale (&movie->original_transform);
  e = movie->yscale / swfdec_matrix_get_yscale (&movie->original_transform);
  cairo_matrix_scale (&movie->matrix, d, e);
  if (isfinite (movie->rotation)) {
    d = movie->rotation - swfdec_matrix_get_rotation (&movie->original_transform);
    cairo_matrix_rotate (&movie->matrix, d * G_PI / 180);
  }
  swfdec_matrix_ensure_invertible (&movie->matrix, &movie->inverse_matrix);

  g_signal_emit (movie, signals[MATRIX_CHANGED], 0);
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
    case SWFDEC_MOVIE_INVALID_EXTENTS:
      swfdec_movie_update_extents (movie);
      /* fall through */
    case SWFDEC_MOVIE_INVALID_CHILDREN:
      break;
    case SWFDEC_MOVIE_UP_TO_DATE:
    default:
      g_assert_not_reached ();
  }
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

SwfdecMovie *
swfdec_movie_find (SwfdecMovie *movie, int depth)
{
  GList *walk;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *cur= walk->data;

    if (cur->depth < depth)
      continue;
    if (cur->depth == depth)
      return cur;
    break;
  }
  return NULL;
}

static void
swfdec_movie_unset_actor (SwfdecPlayer *player, SwfdecActor *actor)
{
  SwfdecPlayerPrivate *priv = player->priv;

  if (priv->mouse_below == actor)
    priv->mouse_below = NULL;
  if (priv->mouse_grab == actor)
    priv->mouse_grab = NULL;
  if (priv->mouse_drag == actor)
    priv->mouse_drag = NULL;

  if (priv->focus_previous == actor)
    priv->focus_previous = NULL;
  if (priv->focus == actor) {
    priv->focus = NULL;
    swfdec_player_invalidate_focusrect (player);
  }
}

static gboolean
swfdec_movie_do_remove (SwfdecMovie *movie, gboolean destroy)
{
  SwfdecPlayer *player;

  SWFDEC_LOG ("removing %s %s", G_OBJECT_TYPE_NAME (movie), movie->name);

  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  while (movie->list) {
    GList *walk = movie->list;
    while (walk && SWFDEC_MOVIE (walk->data)->state >= SWFDEC_MOVIE_STATE_REMOVED)
      walk = walk->next;
    if (walk == NULL)
      break;
    destroy &= swfdec_movie_do_remove (walk->data, destroy);
  }
  /* FIXME: all of this here or in destroy callback? */
  swfdec_movie_invalidate_last (movie);
  movie->state = SWFDEC_MOVIE_STATE_REMOVED;

  if (SWFDEC_IS_ACTOR (movie)) {
    SwfdecActor *actor = SWFDEC_ACTOR (movie);
    swfdec_movie_unset_actor (player, actor);
    if ((actor->events && 
	  swfdec_event_list_has_conditions (actor->events, SWFDEC_EVENT_UNLOAD, 0)) ||
	swfdec_as_object_has_variable (swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (movie)), SWFDEC_AS_STR_onUnload)) {
      swfdec_actor_queue_script (actor, SWFDEC_EVENT_UNLOAD);
      destroy = FALSE;
    }
  }
  if (destroy)
    swfdec_movie_destroy (movie);
  return destroy;
}

/**
 * swfdec_movie_remove:
 * @movie: #SwfdecMovie to remove
 *
 * Removes this movie from its parent. In contrast to swfdec_movie_destroy (),
 * it might still be possible to reference it from Actionscript, if the movie
 * queues onUnload event handlers.
 **/
void
swfdec_movie_remove (SwfdecMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (movie->state > SWFDEC_MOVIE_STATE_RUNNING)
    return;
  if (swfdec_movie_do_remove (movie, TRUE))
    return;
  
  swfdec_movie_set_depth (movie, -32769 - movie->depth); /* don't ask me why... */
}

/**
 * swfdec_movie_destroy:
 * @movie: #SwfdecMovie to destroy
 *
 * Removes this movie from its parent. After this it will no longer be present,
 * neither visually nor via ActionScript. This function will not cause an 
 * unload event. Compare with swfdec_movie_remove ().
 **/
void
swfdec_movie_destroy (SwfdecMovie *movie)
{
  SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (movie);
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));

  g_assert (movie->state < SWFDEC_MOVIE_STATE_DESTROYED);
  SWFDEC_LOG ("destroying movie %s", movie->name);
  while (movie->list) {
    swfdec_movie_destroy (movie->list->data);
  }
  if (movie->parent) {
    movie->parent->list = g_list_remove (movie->parent->list, movie);
  } else {
    player->priv->roots = g_list_remove (player->priv->roots, movie);
  }
  /* unset masks */
  if (movie->masked_by)
    movie->masked_by->mask_of = NULL;
  if (movie->mask_of)
    movie->mask_of->masked_by = NULL;
  movie->masked_by = NULL;
  movie->mask_of = NULL;
  /* FIXME: figure out how to handle destruction pre-init/construct.
   * This is just a stop-gap measure to avoid dead movies in those queues */
  if (SWFDEC_IS_ACTOR (movie))
    swfdec_player_remove_all_actions (player, SWFDEC_ACTOR (movie));
  if (klass->finish_movie)
    klass->finish_movie (movie);
  player->priv->actors = g_list_remove (player->priv->actors, movie);
  if (movie->invalidate_last)
    player->priv->invalid_pending = g_slist_remove (player->priv->invalid_pending, movie);
  movie->state = SWFDEC_MOVIE_STATE_DESTROYED;
  /* remove as value, so we can't be scripted anymore */
  if (movie->as_value) {
    movie->as_value->movie = NULL;
    movie->as_value = NULL;
  }
  g_object_unref (movie);
}

guint
swfdec_movie_get_version (SwfdecMovie *movie)
{
  return movie->resource->version;
}

void
swfdec_movie_local_to_global (SwfdecMovie *movie, double *x, double *y)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  do {
    cairo_matrix_transform_point (&movie->matrix, x, y);
  } while ((movie = movie->parent));
}

void
swfdec_movie_rect_local_to_global (SwfdecMovie *movie, SwfdecRect *rect)
{
  cairo_matrix_t matrix;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (rect != NULL);

  swfdec_movie_local_to_global_matrix (movie, &matrix);
  swfdec_rect_transform (rect, rect, &matrix);
}

void
swfdec_movie_global_to_local_matrix (SwfdecMovie *movie, cairo_matrix_t *matrix)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (matrix != NULL);

  cairo_matrix_init_identity (matrix);
  while (movie) {
    cairo_matrix_multiply (matrix, &movie->inverse_matrix, matrix);
    movie = movie->parent;
  }
}

void
swfdec_movie_local_to_global_matrix (SwfdecMovie *movie, cairo_matrix_t *matrix)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (matrix != NULL);

  cairo_matrix_init_identity (matrix);
  while (movie) {
    cairo_matrix_multiply (matrix, matrix, &movie->matrix);
    movie = movie->parent;
  }
}

void
swfdec_movie_global_to_local (SwfdecMovie *movie, double *x, double *y)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  if (movie->parent) {
    swfdec_movie_global_to_local (movie->parent, x, y);
  }
  cairo_matrix_transform_point (&movie->inverse_matrix, x, y);
}

void
swfdec_movie_rect_global_to_local (SwfdecMovie *movie, SwfdecRect *rect)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (rect != NULL);

  swfdec_movie_global_to_local (movie, &rect->x0, &rect->y0);
  swfdec_movie_global_to_local (movie, &rect->x1, &rect->y1);
  if (rect->x0 > rect->x1) {
    double tmp = rect->x1;
    rect->x1 = rect->x0;
    rect->x0 = tmp;
  }
  if (rect->y0 > rect->y1) {
    double tmp = rect->y1;
    rect->y1 = rect->y0;
    rect->y0 = tmp;
  }
}

/**
 * swfdec_movie_get_mouse:
 * @movie: a #SwfdecMovie
 * @x: pointer to hold result of X coordinate
 * @y: pointer to hold result of y coordinate
 *
 * Gets the mouse coordinates in the coordinate space of @movie.
 **/
void
swfdec_movie_get_mouse (SwfdecMovie *movie, double *x, double *y)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  *x = player->priv->mouse_x;
  *y = player->priv->mouse_y;
  swfdec_player_stage_to_global (player, x, y);
  swfdec_movie_global_to_local (movie, x, y);
}

/**
 * swfdec_movie_get_movie_at:
 * @movie: a #SwfdecMovie
 * @x: x coordinate in parent's coordinate space
 * @y: y coordinate in the parent's coordinate space
 * @events: %TRUE to only prefer movies that receive events
 *
 * Gets the child at the given coordinates. The coordinates are in the 
 * coordinate system of @movie's parent (or the global coordinate system for
 * root movies). The @events parameter determines if movies that don't receive
 * events should be respected.
 *
 * Returns: the child of @movie at the given coordinates or %NULL if none
 **/
SwfdecMovie *
swfdec_movie_get_movie_at (SwfdecMovie *movie, double x, double y, gboolean events)
{
  SwfdecMovie *ret;
  SwfdecMovieClass *klass;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  SWFDEC_LOG ("%s %p getting mouse at: %g %g", G_OBJECT_TYPE_NAME (movie), movie, x, y);
  if (movie->cache_state >= SWFDEC_MOVIE_INVALID_EXTENTS)
      swfdec_movie_update (movie);
  if (!swfdec_rect_contains (&movie->extents, x, y)) {
    return NULL;
  }
  cairo_matrix_transform_point (&movie->inverse_matrix, &x, &y);

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  g_return_val_if_fail (klass->contains, NULL);
  ret = klass->contains (movie, x, y, events);

  return ret;
}

static SwfdecMovie *
swfdec_movie_do_contains (SwfdecMovie *movie, double x, double y, gboolean events)
{
  GList *walk;
  GSList *walk2;
  SwfdecMovie *ret, *got;

  ret = NULL;
  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *child = walk->data;
    
    if (!child->visible) {
      SWFDEC_LOG ("%s %s (depth %d) is invisible, ignoring", G_OBJECT_TYPE_NAME (movie), movie->name, movie->depth);
      continue;
    }
    got = swfdec_movie_get_movie_at (child, x, y, events);
    if (got != NULL) {
      if (events) {
	/* set the return value to the topmost movie */
	if (SWFDEC_IS_ACTOR (got) && swfdec_actor_get_mouse_events (SWFDEC_ACTOR (got))) {
	  ret = got;
	} else if (ret == NULL) {
	  ret = movie;
	}
      } else {
	/* if thie is not a clipped movie, we've found something */
	if (child->clip_depth == 0)
	  return movie;
      }
    } else {
      if (child->clip_depth) {
	/* skip obscured movies */
	SwfdecMovie *tmp = walk->next ? walk->next->data : NULL;
	while (tmp && tmp->depth <= child->clip_depth) {
	  walk = walk->next;
	  tmp = walk->next ? walk->next->data : NULL;
	}
      }
    }
  }
  if (ret)
    return ret;

  for (walk2 = movie->draws; walk2; walk2 = walk2->next) {
    SwfdecDraw *draw = walk2->data;

    if (swfdec_draw_contains (draw, x, y))
      return movie;
  }

  return NULL;
}

/* NB: order is important */
typedef enum {
  SWFDEC_GROUP_NONE = 0,
  SWFDEC_GROUP_NORMAL,
  SWFDEC_GROUP_CACHED,
  SWFDEC_GROUP_FILTERS
} SwfdecGroup;

static SwfdecGroup
swfdec_movie_needs_group (SwfdecMovie *movie)
{
  /* yes, masked movies don't get filters applied */
  if (movie->filters && movie->masked_by == NULL)
    return SWFDEC_GROUP_FILTERS;
  if (movie->cache_as_bitmap)
    return SWFDEC_GROUP_CACHED;
  if (movie->blend_mode > 1)
    return SWFDEC_GROUP_NORMAL;
  return SWFDEC_GROUP_NONE;
}

static cairo_operator_t
swfdec_movie_get_operator_for_blend_mode (guint blend_mode)
{
  switch (blend_mode) {
    case SWFDEC_BLEND_MODE_NONE:
    case SWFDEC_BLEND_MODE_NORMAL:
    case SWFDEC_BLEND_MODE_LAYER:
      return CAIRO_OPERATOR_OVER;
    case SWFDEC_BLEND_MODE_ADD:
      return CAIRO_OPERATOR_ADD;
    case SWFDEC_BLEND_MODE_ALPHA:
      return CAIRO_OPERATOR_DEST_IN;
    case SWFDEC_BLEND_MODE_ERASE:
      return CAIRO_OPERATOR_DEST_OUT;
    case SWFDEC_BLEND_MODE_MULTIPLY:
    case SWFDEC_BLEND_MODE_SCREEN:
    case SWFDEC_BLEND_MODE_LIGHTEN:
    case SWFDEC_BLEND_MODE_DARKEN:
    case SWFDEC_BLEND_MODE_DIFFERENCE:
    case SWFDEC_BLEND_MODE_SUBTRACT:
    case SWFDEC_BLEND_MODE_INVERT:
    case SWFDEC_BLEND_MODE_OVERLAY:
    case SWFDEC_BLEND_MODE_HARDLIGHT:
      SWFDEC_FIXME ("blend mode %u unimplemented in cairo", blend_mode);
      return CAIRO_OPERATOR_OVER;
    default:
      SWFDEC_WARNING ("invalid blend mode %u", blend_mode);
      return CAIRO_OPERATOR_OVER;
  }
}

static cairo_pattern_t *
swfdec_movie_apply_filters (SwfdecMovie *movie, cairo_pattern_t *pattern)
{
  SwfdecRectangle area;
  SwfdecPlayer *player;
  SwfdecRect rect;
  GSList *walk;
  double xscale, yscale;

  if (movie->filters == NULL)
    return pattern;

  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  rect = movie->original_extents;
  swfdec_movie_rect_local_to_global (movie, &rect);
  swfdec_rect_transform (&rect, &rect,
      &player->priv->global_to_stage);
  swfdec_rectangle_init_rect (&area, &rect);
  /* FIXME: hack to make textfield borders work - looks like Adobe does this, too */
  area.width++;
  area.height++;
  xscale = player->priv->global_to_stage.xx * SWFDEC_TWIPS_SCALE_FACTOR;
  yscale = player->priv->global_to_stage.yy * SWFDEC_TWIPS_SCALE_FACTOR;
  for (walk = movie->filters; walk; walk = walk->next) {
    pattern = swfdec_filter_apply (walk->data, pattern, xscale, yscale, &area);
    swfdec_filter_get_rectangle (walk->data, &area, xscale, yscale, &area);
  }
  return pattern;
}

/**
 * swfdec_movie_mask:
 * @movie: The movie to act as the mask
 * @cr: a cairo context which should be used for masking. The cairo context's
 *      matrix is assumed to be in the coordinate system of the movie's parent.
 * @matrix: matrix to apply before rendering
 *
 * Creates a pattern suitable for masking. To do rendering using the returned
 * mask, you want to use code like this:
 * <informalexample><programlisting>
 * mask = swfdec_movie_mask (cr, movie, matrix);
 * cairo_push_group (cr);
 * // do rendering here
 * cairo_pop_group_to_source (cr);
 * cairo_mask (cr, mask);
 * cairo_pattern_destroy (mask);
 * </programlisting></informalexample>
 *
 * Returns: A new cairo_patten_t to be used as the mask.
 **/
static cairo_pattern_t *
swfdec_movie_mask (cairo_t *cr, SwfdecMovie *movie,
    const cairo_matrix_t *matrix)
{
  SwfdecColorTransform black;

  swfdec_color_transform_init_mask (&black);
  cairo_push_group_with_content (cr, CAIRO_CONTENT_ALPHA);
  cairo_transform (cr, matrix);

  swfdec_movie_render (movie, cr, &black);
  return cairo_pop_group (cr);
}

void
swfdec_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *color_transform)
{
  SwfdecMovieClass *klass;
  SwfdecColorTransform trans;
  SwfdecGroup group;
  gboolean needs_mask;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (cr != NULL);
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
    g_warning ("%s", cairo_status_to_string (cairo_status (cr)));
  }
  g_return_if_fail (color_transform != NULL);
  
  if (movie->mask_of != NULL && !swfdec_color_transform_is_mask (color_transform)) {
    SWFDEC_LOG ("not rendering %s %p, movie is a mask",
	G_OBJECT_TYPE_NAME (movie), movie->name);
    return;
  }

  group = swfdec_movie_needs_group (movie);
  if (group == SWFDEC_GROUP_NORMAL) {
    SWFDEC_DEBUG ("pushing group for blend mode %u", movie->blend_mode);
    cairo_push_group (cr);
  } else if (group != SWFDEC_GROUP_NONE) {
    SWFDEC_FIXME ("implement cache-as-bitmap here");
    cairo_push_group (cr);
  }
  /* yes, movie with filters, don't get masked */
  needs_mask = movie->masked_by != NULL && movie->filters == NULL;
  if (needs_mask) {
    cairo_push_group (cr);
  }

  /* do extra save/restore so the render vfunc can mess with cr */
  cairo_save (cr);

  SWFDEC_LOG ("transforming movie, transform: %g %g  %g %g   %g %g",
      movie->matrix.xx, movie->matrix.yy,
      movie->matrix.xy, movie->matrix.yx,
      movie->matrix.x0, movie->matrix.y0);
  cairo_transform (cr, &movie->matrix);
  swfdec_color_transform_chain (&trans, &movie->color_transform, color_transform);

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  g_return_if_fail (klass->render);
  klass->render (movie, cr, &trans);
#if 0
  /* code to draw a red rectangle around the area occupied by this movie clip */
  {
    cairo_save (cr);
    cairo_transform (cr, &movie->inverse_matrix);
    cairo_rectangle (cr, movie->extents.x0, movie->extents.y0,
	movie->extents.x1 - movie->extents.x0,
	movie->extents.y1 - movie->extents.y0);
    swfdec_renderer_reset_matrix (cr);
    cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
    cairo_set_line_width (cr, 2.0);
    cairo_stroke (cr);
    cairo_restore (cr);
  }
#endif
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
    g_warning ("error rendering with cairo: %s", cairo_status_to_string (cairo_status (cr)));
  }
  cairo_restore (cr);

  if (needs_mask) {
    cairo_pattern_t *mask;
    cairo_matrix_t mat;
    if (movie->parent)
      swfdec_movie_global_to_local_matrix (movie->parent, &mat);
    else
      cairo_matrix_init_identity (&mat);
    if (movie->masked_by->parent) {
      cairo_matrix_t mat2;
      swfdec_movie_local_to_global_matrix (movie->masked_by->parent, &mat2);
      cairo_matrix_multiply (&mat, &mat, &mat2);
    }
    mask = swfdec_movie_mask (cr, movie->masked_by, &mat);
    cairo_pop_group_to_source (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_mask (cr, mask);
    cairo_pattern_destroy (mask);
  }
  if (group == SWFDEC_GROUP_FILTERS) {
    cairo_pattern_t *pattern;

    pattern = cairo_pop_group (cr);
    cairo_save (cr);
    swfdec_renderer_reset_matrix (cr);
    {
      cairo_matrix_t mat;
      cairo_get_matrix (cr, &mat);
      cairo_matrix_invert (&mat);
      cairo_pattern_set_matrix (pattern, &mat);
    }
    pattern = swfdec_movie_apply_filters (movie, pattern);
    cairo_set_source (cr, pattern);
    cairo_set_operator (cr, swfdec_movie_get_operator_for_blend_mode (movie->blend_mode));
    cairo_paint (cr);
    cairo_pattern_destroy (pattern);
    cairo_restore (cr);
  } else if (group != SWFDEC_GROUP_NONE) {
    cairo_pop_group_to_source (cr);
    cairo_set_operator (cr, swfdec_movie_get_operator_for_blend_mode (movie->blend_mode));
    cairo_paint (cr);
  }
}

static void
swfdec_movie_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);

  switch (param_id) {
    case PROP_DEPTH:
      g_value_set_int (value, movie->depth);
      break;
    case PROP_PARENT:
      g_value_set_object (value, movie->parent);
      break;
    case PROP_RESOURCE:
      g_value_set_object (value, movie->resource);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_movie_set_property (GObject *object, guint param_id, const GValue *value, 
    GParamSpec * pspec)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);
  SwfdecAsContext *cx = swfdec_gc_object_get_context (movie);

  /* The context must be set before all movie-related properties */
  g_assert (cx);

  switch (param_id) {
    case PROP_DEPTH:
      /* parent must be set after depth */
      g_assert (movie->parent == NULL);
      movie->depth = g_value_get_int (value);
      break;
    case PROP_GRAPHIC:
      movie->graphic = g_value_get_object (value);
      if (movie->graphic)
	g_object_ref (movie->graphic);
      break;
    case PROP_NAME:
      movie->name = g_value_get_string (value);
      if (movie->name) {
	movie->name = swfdec_as_context_get_string (cx, movie->name);
      } else {
	movie->name = SWFDEC_AS_STR_EMPTY;
      }
      break;
    case PROP_PARENT:
      movie->parent = g_value_get_object (value);
      /* parent holds a reference */
      g_object_ref (movie);
      if (movie->parent) {
	movie->parent->list = g_list_insert_sorted (movie->parent->list, movie, swfdec_movie_compare_depths);
	SWFDEC_DEBUG ("inserting %s %p into %s %p", G_OBJECT_TYPE_NAME (movie), movie,
	    G_OBJECT_TYPE_NAME (movie->parent), movie->parent);
	/* invalidate the parent, so it gets visible */
	swfdec_movie_queue_update (movie->parent, SWFDEC_MOVIE_INVALID_CHILDREN);
      } else {
	SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;
	priv->roots = g_list_insert_sorted (priv->roots, movie, swfdec_movie_compare_depths);
      }
      break;
    case PROP_RESOURCE:
      movie->resource = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_movie_dispose (GObject *object)
{
  SwfdecMovie * movie = SWFDEC_MOVIE (object);
  GSList *iter;

  g_assert (movie->list == NULL);

  SWFDEC_LOG ("disposing movie %s (depth %d)", movie->name, movie->depth);
  if (movie->graphic) {
    g_object_unref (movie->graphic);
    movie->graphic = NULL;
  }
  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    g_free (iter->data);
  }
  g_slist_free (movie->variable_listeners);
  movie->variable_listeners = NULL;

  if (movie->image) {
    g_object_unref (movie->image);
    movie->image = NULL;
  }
  g_slist_foreach (movie->draws, (GFunc) g_object_unref, NULL);
  g_slist_free (movie->draws);
  movie->draws = NULL;

  G_OBJECT_CLASS (swfdec_movie_parent_class)->dispose (G_OBJECT (movie));
}

static void
swfdec_movie_mark (SwfdecGcObject *object)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);
  GSList *iter;

  if (movie->parent)
    swfdec_gc_object_mark (movie->parent);
  if (movie->as_value)
    swfdec_as_movie_value_mark (movie->as_value);
  swfdec_as_string_mark (movie->name);
  g_list_foreach (movie->list, (GFunc) swfdec_gc_object_mark, NULL);
  g_slist_foreach (movie->filters, (GFunc) swfdec_gc_object_mark, NULL);

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    SwfdecMovieVariableListener *listener = iter->data;
    swfdec_as_string_mark (listener->name);
  }
  swfdec_gc_object_mark (movie->resource);

  SWFDEC_GC_OBJECT_CLASS (swfdec_movie_parent_class)->mark (object);
}

/**
 * swfdec_movie_is_scriptable:
 * @movie: a movie
 *
 * Checks if the movie may be accessed by scripts. If not, the movie is not
 * accessible by Actionscript and functions that would return the movie should
 * instead return its parent.
 *
 * Returns: %TRUE if scripts may access this movie, %FALSE if the parent 
 *          should be used.
 **/
gboolean
swfdec_movie_is_scriptable (SwfdecMovie *movie)
{
  /* FIXME: It would be much easier if we'd just check that there's no as_value 
   * for non-scriptable movies */
  return (SWFDEC_IS_ACTOR (movie) || SWFDEC_IS_VIDEO_MOVIE (movie)) &&
   (swfdec_movie_get_version (movie) > 5 || !SWFDEC_IS_TEXT_FIELD_MOVIE (movie));
}

/* FIXME: This function can definitely be implemented easier */
SwfdecMovie *
swfdec_movie_get_by_name (SwfdecMovie *movie, const char *name, gboolean unnamed)
{
  GList *walk;
  int i;
  guint version = swfdec_gc_object_get_context (movie)->version;
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));

  if (name[0] == '\0')
    return NULL;

  i = swfdec_player_get_level (player, name, version);
  if (i >= 0)
    return SWFDEC_MOVIE (swfdec_player_get_movie_at_level (player, i));

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (swfdec_strcmp (version, cur->name, name) == 0) {
      if (swfdec_movie_is_scriptable (cur))
	return cur;
      else
	return movie;
    }
    if (unnamed && cur->name == SWFDEC_AS_STR_EMPTY && cur->as_value) {
      if (swfdec_strcmp (version, cur->as_value->names[cur->as_value->n_names - 1], name) == 0) {
	/* unnamed movies are always scriptable */
	return cur;
      }
    }
  }
  return NULL;
}

SwfdecMovie *
swfdec_movie_get_root (SwfdecMovie *movie)
{
  SwfdecMovie *real_root;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  real_root = movie;
  while (real_root->parent)
    real_root = real_root->parent;

  while (movie->parent && !(movie->lockroot &&
	(swfdec_movie_get_version (movie) != 6 ||
	 swfdec_movie_get_version (real_root) != 6))) {
    movie = movie->parent;
  }

  return movie;
}

void
swfdec_movie_add_variable_listener (SwfdecMovie *movie, gpointer data,
    const char *name, const SwfdecMovieVariableListenerFunction function)
{
  SwfdecMovieVariableListener *listener;
  GSList *iter;

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    listener = iter->data;

    if (listener->data == data && listener->name == name &&
	listener->function == function)
      return;
  }

  listener = g_new0 (SwfdecMovieVariableListener, 1);
  listener->data = data;
  listener->name = name;
  listener->function = function;

  movie->variable_listeners = g_slist_prepend (movie->variable_listeners,
      listener);
}

void
swfdec_movie_remove_variable_listener (SwfdecMovie *movie,
    gpointer data, const char *name,
    const SwfdecMovieVariableListenerFunction function)
{
  GSList *iter;

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    SwfdecMovieVariableListener *listener = iter->data;

    if (listener->data == data && listener->name == name &&
	listener->function == function)
      break;
  }
  if (iter == NULL)
    return;

  g_free (iter->data);
  movie->variable_listeners =
    g_slist_remove (movie->variable_listeners, iter->data);
}

void
swfdec_movie_call_variable_listeners (SwfdecMovie *movie, const char *name,
    const SwfdecAsValue *val)
{
  GSList *iter;

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    SwfdecMovieVariableListener *listener = iter->data;

    if (listener->name != name &&
	(swfdec_gc_object_get_context (movie)->version >= 7 ||
	 !swfdec_str_case_equal (listener->name, name)))
      continue;

    listener->function (listener->data, name, val);
  }
}

typedef struct {
  SwfdecMovie *		movie;
  int			depth;
} ClipEntry;

static void
swfdec_movie_do_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *ctrans)
{
  static const cairo_matrix_t ident = { 1, 0, 0, 1, 0, 0};
  GList *g;
  GSList *walk;
  GSList *clips = NULL;
  ClipEntry *clip = NULL;

  if (movie->draws || movie->image) {
    SwfdecRect inval;

    cairo_clip_extents (cr, &inval.x0, &inval.y0, &inval.x1, &inval.y1);

    /* exeute the movie's drawing commands */
    for (walk = movie->draws; walk; walk = walk->next) {
      SwfdecDraw *draw = walk->data;

      if (!swfdec_rect_intersect (NULL, &draw->extents, &inval))
	continue;
      
      swfdec_draw_paint (draw, cr, ctrans);
    }

    /* if the movie loaded an image, draw it here now */
    /* FIXME: add check to only draw if inside clip extents */
    if (movie->image) {
      SwfdecRenderer *renderer = swfdec_renderer_get (cr);
      cairo_surface_t *surface;
      cairo_pattern_t *pattern;
      double alpha = 1.0;
      
      if (swfdec_color_transform_is_mask (ctrans)) {
	surface = NULL;
      } else if (swfdec_color_transform_is_alpha (ctrans)) {
	surface = swfdec_image_create_surface (movie->image, renderer);
	alpha = ctrans->aa / 256.0;
      } else {
	surface = swfdec_image_create_surface_transformed (movie->image,
	  renderer, ctrans);
      }
      if (surface) {
	static const cairo_matrix_t matrix = { 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 0, 0, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 0, 0 };
	pattern = cairo_pattern_create_for_surface (surface);
	SWFDEC_LOG ("rendering loaded image");
	cairo_pattern_set_matrix (pattern, &matrix);
      } else {
	pattern = cairo_pattern_create_rgb (1.0, 0.0, 0.0);
      }
      cairo_set_source (cr, pattern);
      cairo_paint_with_alpha (cr, alpha);
      cairo_pattern_destroy (pattern);
      cairo_surface_destroy (surface);
    }
  }

  /* draw the children movies */
  for (g = movie->list; g; g = g_list_next (g)) {
    SwfdecMovie *child = g->data;

    while (clip && clip->depth < child->depth) {
      cairo_pattern_t *mask;
      SWFDEC_INFO ("unsetting clip depth %d for depth %d", clip->depth, child->depth);
      mask = swfdec_movie_mask (cr, clip->movie, &ident);
      cairo_pop_group_to_source (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_mask (cr, mask);
      cairo_pattern_destroy (mask);
      g_slice_free (ClipEntry, clip);
      clips = g_slist_delete_link (clips, clips);
      clip = clips ? clips->data : NULL;
    }

    if (child->clip_depth) {
      clip = g_slice_new (ClipEntry);
      clips = g_slist_prepend (clips, clip);
      clip->movie = child;
      clip->depth = child->clip_depth;
      SWFDEC_INFO ("clipping up to depth %d by using %s with depth %d", child->clip_depth,
	  child->name, child->depth);
      cairo_push_group (cr);
      continue;
    }

    SWFDEC_LOG ("rendering %p with depth %d", child, child->depth);
    if (child->visible)
      swfdec_movie_render (child, cr, ctrans);
  }
  while (clip) {
    cairo_pattern_t *mask;
    SWFDEC_INFO ("unsetting clip depth %d", clip->depth);
    mask = swfdec_movie_mask (cr, clip->movie, &ident);
    cairo_pop_group_to_source (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_mask (cr, mask);
    cairo_pattern_destroy (mask);
    g_slice_free (ClipEntry, clip);
    clips = g_slist_delete_link (clips, clips);
    clip = clips ? clips->data : NULL;
  }
  g_assert (clips == NULL);
}

static void
swfdec_movie_do_invalidate (SwfdecMovie *movie, const cairo_matrix_t *matrix, gboolean last)
{
  GList *walk;
  SwfdecRect rect;

  if (movie->image) {
    rect.x0 = rect.y0 = 0;
    rect.x1 = movie->image->width * SWFDEC_TWIPS_SCALE_FACTOR;
    rect.y1 = movie->image->height * SWFDEC_TWIPS_SCALE_FACTOR;
  } else {
    swfdec_rect_init_empty (&rect);
  }
  swfdec_rect_union (&rect, &rect, &movie->draw_extents);
  swfdec_rect_transform (&rect, &rect, matrix);
  swfdec_player_invalidate (SWFDEC_PLAYER (swfdec_gc_object_get_context (movie)),
      movie, &rect);

  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_movie_invalidate (walk->data, matrix, last);
  }
}

static GObject *
swfdec_movie_constructor (GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties)
{
  SwfdecPlayerPrivate *priv;
  SwfdecAsContext *cx;
  SwfdecAsObject *o;
  SwfdecMovie *movie;
  GObject *object;
  const char *name;

  object = G_OBJECT_CLASS (swfdec_movie_parent_class)->constructor (type, 
      n_construct_properties, construct_properties);
  movie = SWFDEC_MOVIE (object);

  cx = swfdec_gc_object_get_context (movie);
  priv = SWFDEC_PLAYER (cx)->priv;
  /* the movie is created invalid */
  priv->invalid_pending = g_slist_prepend (priv->invalid_pending, object);

  if (movie->name == SWFDEC_AS_STR_EMPTY && 
      (swfdec_movie_is_scriptable (movie) || SWFDEC_IS_ACTOR (movie))) {
    name = swfdec_as_context_give_string (cx,
	g_strdup_printf ("instance%u", ++priv->unnamed_count));
  } else {
    name = movie->name;
  }
  if (name != SWFDEC_AS_STR_EMPTY)
    movie->as_value = swfdec_as_movie_value_new (movie, name);

  /* make the resource ours if it doesn't belong to anyone yet */
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (movie->resource->movie)) {
    g_assert (SWFDEC_IS_SPRITE_MOVIE (movie));
    SWFDEC_AS_VALUE_SET_MOVIE (&movie->resource->movie, movie);
  }

  /* create AsObject */
  o = swfdec_as_object_new_empty (cx);
  o->movie = TRUE;
  swfdec_as_object_set_relay (o, SWFDEC_AS_RELAY (movie));

  /* set $version variable */
  if (movie->parent == NULL) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, priv->system->version));
    swfdec_as_object_set_variable (o, SWFDEC_AS_STR__version, &val);
  }
  return object;
}

static void
swfdec_movie_class_init (SwfdecMovieClass * movie_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (movie_class);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (movie_class);

  object_class->constructor = swfdec_movie_constructor;
  object_class->dispose = swfdec_movie_dispose;
  object_class->get_property = swfdec_movie_get_property;
  object_class->set_property = swfdec_movie_set_property;

  gc_class->mark = swfdec_movie_mark;

  signals[MATRIX_CHANGED] = g_signal_new ("matrix-changed", G_TYPE_FROM_CLASS (movie_class),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_DEPTH,
      g_param_spec_int ("depth", "depth", "z order inside the parent",
	  G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_GRAPHIC,
      g_param_spec_object ("graphic", "graphic", "graphic represented by this movie",
	  SWFDEC_TYPE_GRAPHIC, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", "name", "the name given to this movie (can be empty)",
	  NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_PARENT,
      g_param_spec_object ("parent", "parent", "parent movie containing this movie",
	  SWFDEC_TYPE_MOVIE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_RESOURCE,
      g_param_spec_object ("resource", "resource", "the resource that spawned this movie",
	  SWFDEC_TYPE_RESOURCE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  movie_class->render = swfdec_movie_do_render;
  movie_class->invalidate = swfdec_movie_do_invalidate;
  movie_class->contains = swfdec_movie_do_contains;
  movie_class->property_get = swfdec_movie_property_do_get;
  movie_class->property_set = swfdec_movie_property_do_set;
}

void
swfdec_movie_initialize (SwfdecMovie *movie)
{
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->init_movie)
    klass->init_movie (movie);
}

void
swfdec_movie_set_depth (SwfdecMovie *movie, int depth)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (movie->depth == depth)
    return;

  swfdec_movie_invalidate_last (movie);
  movie->depth = depth;
  if (movie->parent) {
    movie->parent->list = g_list_sort (movie->parent->list, swfdec_movie_compare_depths);
  } else {
    SwfdecPlayerPrivate *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie))->priv;
    player->roots = g_list_sort (player->roots, swfdec_movie_compare_depths);
  }
  g_object_notify (G_OBJECT (movie), "depth");
}

/**
 * swfdec_movie_new:
 * @player: a #SwfdecPlayer
 * @depth: depth of movie
 * @parent: the parent movie or %NULL to make this a root movie
 * @resource: the resource that is responsible for this movie
 * @graphic: the graphic that is displayed by this movie or %NULL to create an 
 *           empty movieclip
 * @name: a garbage-collected string to be used as the name for this movie or 
 *        %NULL for a default one.
 *
 * Creates a new movie #SwfdecMovie for the given properties. No movie may exist
 * at the given @depth. The actual type of
 * this movie depends on the @graphic parameter. The movie will be initialized 
 * with default properties. No script execution will be scheduled. After all 
 * properties are set, the new-movie signal will be emitted if @player is a 
 * debugger.
 *
 * Returns: a new #SwfdecMovie
 **/
SwfdecMovie *
swfdec_movie_new (SwfdecPlayer *player, int depth, SwfdecMovie *parent, SwfdecResource *resource,
    SwfdecGraphic *graphic, const char *name)
{
  SwfdecMovie *movie;
  GType type;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (parent == NULL || SWFDEC_IS_MOVIE (parent), NULL);
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (resource), NULL);
  g_return_val_if_fail (graphic == NULL || SWFDEC_IS_GRAPHIC (graphic), NULL);

  /* create the right movie */
  if (graphic == NULL) {
    type = SWFDEC_TYPE_SPRITE_MOVIE;
  } else {
    SwfdecGraphicClass *klass = SWFDEC_GRAPHIC_GET_CLASS (graphic);
    g_return_val_if_fail (g_type_is_a (klass->movie_type, SWFDEC_TYPE_MOVIE), NULL);
    type = klass->movie_type;
  }
  movie = g_object_new (type, "context", player, "depth", depth, 
      "parent", parent, "name", name, "resource", resource,
      "graphic", graphic, NULL);

  return movie;
}

/* FIXME: since this is only used in PlaceObject, wouldn't it be easier to just have
 * swfdec_movie_update_static_properties (movie); that's notified when any of these change
 * and let PlaceObject modify the movie directly?
 */
void
swfdec_movie_set_static_properties (SwfdecMovie *movie, const cairo_matrix_t *transform,
    const SwfdecColorTransform *ctrans, int ratio, int clip_depth, guint blend_mode,
    SwfdecEventList *events)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (clip_depth >= -16384 || clip_depth <= 0);
  g_return_if_fail (ratio >= -1);

  if (movie->modified) {
    SWFDEC_LOG ("%s has already been modified by scripts, ignoring updates", movie->name);
    return;
  }
  if (transform) {
    swfdec_movie_begin_update_matrix (movie);
    movie->original_transform = *transform;
    movie->matrix.x0 = movie->original_transform.x0;
    movie->matrix.y0 = movie->original_transform.y0;
    movie->xscale = swfdec_matrix_get_xscale (&movie->original_transform);
    movie->yscale = swfdec_matrix_get_yscale (&movie->original_transform);
    movie->rotation = swfdec_matrix_get_rotation (&movie->original_transform);
    swfdec_movie_end_update_matrix (movie);
  }
  if (ctrans) {
    swfdec_movie_invalidate_last (movie);
    movie->color_transform = *ctrans;
  }
  if (ratio >= 0 && (guint) ratio != movie->original_ratio) {
    SwfdecMovieClass *klass;
    movie->original_ratio = ratio;
    klass = SWFDEC_MOVIE_GET_CLASS (movie);
    if (klass->set_ratio)
      klass->set_ratio (movie);
  }
  if (clip_depth && clip_depth != movie->clip_depth) {
    movie->clip_depth = clip_depth;
    /* FIXME: is this correct? */
    swfdec_movie_invalidate_last (movie->parent ? movie->parent : movie);
  }
  if (blend_mode != movie->blend_mode) {
    movie->blend_mode = blend_mode;
    swfdec_movie_invalidate_last (movie);
  }
  if (events) {
    if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
      SwfdecActor *actor = SWFDEC_ACTOR (movie);
      if (actor->events)
	swfdec_event_list_free (actor->events);
      actor->events = swfdec_event_list_copy (events);
    } else {
      SWFDEC_WARNING ("trying to set events on a %s, not allowed", G_OBJECT_TYPE_NAME (movie));
    }
  }
}

/**
 * swfdec_movie_duplicate:
 * @movie: #SwfdecMovie to copy
 * @name: garbage-collected name for the new copy
 * @depth: depth to put this movie in
 *
 * Creates a duplicate of @movie. The duplicate will not be initialized or
 * queued up for any events. You have to do this manually. In particular calling
 * swfdec_movie_initialize() on the returned movie must be done.
 * This function must be called from within a script.
 *
 * Returns: a newly created movie or %NULL on error
 **/
SwfdecMovie *
swfdec_movie_duplicate (SwfdecMovie *movie, const char *name, int depth)
{
  SwfdecMovie *parent, *copy;
  SwfdecSandbox *sandbox;
  GSList *walk;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  parent = movie->parent;
  if (movie->parent == NULL) {
    SWFDEC_FIXME ("don't know how to duplicate root movies");
    return NULL;
  }
  copy = swfdec_movie_find (movie->parent, depth);
  if (copy) {
    SWFDEC_LOG ("depth %d already occupied while duplicating, removing old movie", depth);
    swfdec_movie_remove (copy);
  }
  copy = swfdec_movie_new (SWFDEC_PLAYER (swfdec_gc_object_get_context (movie)), depth, 
      parent, movie->resource, movie->graphic, name);
  /* copy properties */
  swfdec_movie_set_static_properties (copy, &movie->original_transform,
      &movie->color_transform, movie->original_ratio, movie->clip_depth, 
      movie->blend_mode, SWFDEC_IS_ACTOR (movie) ? SWFDEC_ACTOR (movie)->events : NULL);
  /* Copy drawing state.
   * We can keep refs to all finalized draw objects, but need to create copies
   * of the still active ones as their path can still change */
  copy->draws = g_slist_copy (movie->draws);
  g_slist_foreach (copy->draws, (GFunc) g_object_ref, NULL);
  copy->draw_extents = movie->draw_extents;
  for (walk = copy->draws; walk; walk = walk->next) {
    if (walk->data == movie->draw_line) {
      copy->draw_line = swfdec_draw_copy (walk->data);
      g_object_unref (walk->data);
      walk->data = copy->draw_line;
    } else if (walk->data == movie->draw_fill) {
      copy->draw_fill = swfdec_draw_copy (walk->data);
      g_object_unref (walk->data);
      walk->data = copy->draw_fill;
    }
  }
  copy->draw_x = movie->draw_x;
  copy->draw_y = movie->draw_y;
  g_assert (copy->cache_state >= SWFDEC_MOVIE_INVALID_EXTENTS);

  sandbox = swfdec_sandbox_get (SWFDEC_PLAYER (swfdec_gc_object_get_context (movie)));
  swfdec_sandbox_unuse (sandbox);
  if (SWFDEC_IS_SPRITE_MOVIE (copy)) {
    SwfdecActor *actor = SWFDEC_ACTOR (copy);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_INITIALIZE);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_LOAD);
    swfdec_actor_execute (actor, SWFDEC_EVENT_CONSTRUCT, 0);
  }
  swfdec_movie_initialize (copy);
  swfdec_sandbox_use (sandbox);
  return copy;
}

char *
swfdec_movie_get_path (SwfdecMovie *movie, gboolean dot)
{
  GString *s;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);
  
  s = g_string_new ("");
  do {
    if (movie->parent) {
      if (movie->name != SWFDEC_AS_STR_EMPTY) {
	g_string_prepend (s, movie->name);
      } else if (movie->as_value) {
	g_string_prepend (s, movie->as_value->names[movie->as_value->n_names - 1]);
      } else {
	g_assert_not_reached ();
      }
      g_string_prepend_c (s, (dot ? '.' : '/'));
    } else {
      char *ret;
      if (dot) {
	ret = g_strdup_printf ("_level%u%s", movie->depth + 16384, s->str);
	g_string_free (s, TRUE);
      } else {
	if (s->str[0] != '/')
	  g_string_prepend_c (s, '/');
	ret = g_string_free (s, FALSE);
      }
      return ret;
    }
    movie = movie->parent;
  } while (TRUE);

  g_assert_not_reached ();

  return NULL;
}

int
swfdec_movie_compare_depths (gconstpointer a, gconstpointer b)
{
  if (SWFDEC_MOVIE (a)->depth < SWFDEC_MOVIE (b)->depth)
    return -1;
  if (SWFDEC_MOVIE (a)->depth > SWFDEC_MOVIE (b)->depth)
    return 1;
  return 0;
}

/**
 * swfdec_depth_classify:
 * @depth: the depth to classify
 *
 * Classifies a depth. This classification is mostly used when deciding if
 * certain operations are valid in ActionScript.
 *
 * Returns: the classification of the depth.
 **/
SwfdecDepthClass
swfdec_depth_classify (int depth)
{
  if (depth < -16384)
    return SWFDEC_DEPTH_CLASS_EMPTY;
  if (depth < 0)
    return SWFDEC_DEPTH_CLASS_TIMELINE;
  if (depth < 1048576)
    return SWFDEC_DEPTH_CLASS_DYNAMIC;
  if (depth < 2130690046)
    return SWFDEC_DEPTH_CLASS_RESERVED;
  return SWFDEC_DEPTH_CLASS_EMPTY;
}

/**
 * swfdec_movie_get_own_resource:
 * @movie: movie to query
 *
 * Queries the movie for his own resource. A movie only has its own resource if
 * it contains data loaded with the loadMovie() function, or if it is the root
 * movie.
 *
 * Returns: The own resource of @movie or %NULL
 **/
SwfdecResource *
swfdec_movie_get_own_resource (SwfdecMovie *movie)
{
  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  if (!SWFDEC_IS_SPRITE_MOVIE (movie))
    return NULL;

  if (SWFDEC_AS_VALUE_GET_MOVIE (movie->resource->movie) != movie)
    return NULL;

  return movie->resource;
}

void
swfdec_movie_property_set (SwfdecMovie *movie, guint id, SwfdecAsValue val)
{
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  klass->property_set (movie, id, val);
}

SwfdecAsValue
swfdec_movie_property_get (SwfdecMovie *movie, guint id)
{
  SwfdecMovieClass *klass;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), SWFDEC_AS_VALUE_UNDEFINED);

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  return klass->property_get (movie, id);
}

