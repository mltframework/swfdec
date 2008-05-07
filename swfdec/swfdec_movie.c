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
#include <strings.h>
#include <errno.h>

#include "swfdec_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_button_movie.h"
#include "swfdec_debug.h"
#include "swfdec_draw.h"
#include "swfdec_event.h"
#include "swfdec_graphic.h"
#include "swfdec_image.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_resource.h"
#include "swfdec_system.h"
#include "swfdec_text_field_movie.h"
#include "swfdec_utils.h"
#include "swfdec_video_movie.h"

/*** MOVIE ***/

enum {
  PROP_0,
  PROP_DEPTH
};

G_DEFINE_ABSTRACT_TYPE (SwfdecMovie, swfdec_movie, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_movie_init (SwfdecMovie * movie)
{
  movie->blend_mode = 1;

  movie->xscale = 100;
  movie->yscale = 100;
  cairo_matrix_init_identity (&movie->original_transform);
  cairo_matrix_init_identity (&movie->matrix);
  cairo_matrix_init_identity (&movie->inverse_matrix);

  swfdec_color_transform_init_identity (&movie->color_transform);
  swfdec_color_transform_init_identity (&movie->original_ctrans);

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
    player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
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
  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
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

  if (state > SWFDEC_MOVIE_INVALID_EXTENTS) {
    swfdec_movie_invalidate_next (movie);
  }
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

static void
swfdec_movie_update_matrix (SwfdecMovie *movie)
{
  double d, e;

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
    case SWFDEC_MOVIE_INVALID_MATRIX:
      swfdec_movie_update_matrix (movie);
      /* fall through */
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

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
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
	  swfdec_event_list_has_conditions (actor->events, SWFDEC_AS_OBJECT (movie), SWFDEC_EVENT_UNLOAD, 0)) ||
	swfdec_as_object_has_variable (SWFDEC_AS_OBJECT (movie), SWFDEC_AS_STR_onUnload)) {
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
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);

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
  /* unset prototype here, so we don't work in AS anymore */
  SWFDEC_AS_OBJECT (movie)->prototype = NULL;
  g_object_unref (movie);
}

/**
 * swfdec_movie_resolve:
 * @movie: movie to resolve
 *
 * Resolves a movie clip to its real version. Since movie clips can be 
 * explicitly destroyed, they have problems with references to them. In the
 * case of destruction, these references will remain as "dangling pointers".
 * However, if a movie with the same name is later created again, the reference
 * will point to that movie. This function does this resolving.
 *
 * Returns: The movie clip @movie resolves to or %NULL if none.
 **/
SwfdecMovie *
swfdec_movie_resolve (SwfdecMovie *movie)
{
  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  if (movie->state != SWFDEC_MOVIE_STATE_DESTROYED)
    return movie;
  if (movie->parent == NULL) {
    SWFDEC_FIXME ("figure out how to resolve root movies");
    return NULL;
  }
  /* FIXME: include unnamed ones? */
  return swfdec_movie_get_by_name (movie->parent, movie->original_name, FALSE);
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
    if (movie->cache_state >= SWFDEC_MOVIE_INVALID_MATRIX)
      swfdec_movie_update (movie);
    cairo_matrix_transform_point (&movie->matrix, x, y);
  } while ((movie = movie->parent));
}

void
swfdec_movie_rect_local_to_global (SwfdecMovie *movie, SwfdecRect *rect)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (rect != NULL);

  swfdec_movie_local_to_global (movie, &rect->x0, &rect->y0);
  swfdec_movie_local_to_global (movie, &rect->x1, &rect->y1);
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

void
swfdec_movie_global_to_local_matrix (SwfdecMovie *movie, cairo_matrix_t *matrix)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (matrix != NULL);

  cairo_matrix_init_identity (matrix);
  while (movie) {
    if (movie->cache_state >= SWFDEC_MOVIE_INVALID_MATRIX)
      swfdec_movie_update (movie);
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
    if (movie->cache_state >= SWFDEC_MOVIE_INVALID_MATRIX)
      swfdec_movie_update (movie);
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
  if (movie->cache_state >= SWFDEC_MOVIE_INVALID_MATRIX)
    swfdec_movie_update (movie);
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

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
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

static gboolean
swfdec_movie_needs_group (SwfdecMovie *movie)
{
  return (movie->blend_mode > 1);
}

static cairo_operator_t
swfdec_movie_get_operator_for_blend_mode (guint blend_mode)
{
  switch (blend_mode) {
    case SWFDEC_BLEND_MODE_NORMAL:
      SWFDEC_ERROR ("shouldn't need to get operator without blend mode?!");
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
      SWFDEC_WARNING ("blend mode %u unimplemented in cairo", blend_mode);
      return CAIRO_OPERATOR_OVER;
    default:
      SWFDEC_WARNING ("invalid blend mode %u", blend_mode);
      return CAIRO_OPERATOR_OVER;
  }
}

/**
 * swfdec_movie_mask:
 * @movie: The movie to act as the mask
 * @cr: a cairo context which should be used for masking. The cairo context's
 *      matrix is assumed to be in the coordinate system of the movie's parent.
 * @inval: The region that is relevant for masking
 *
 * Creates a pattern suitable for masking. To do rendering using the returned
 * mask, you want to use code like this:
 * <informalexample><programlisting>
 * mask = swfdec_movie_mask (cr, movie, inval);
 * cairo_push_group (cr);
 * // do rendering here
 * cairo_pop_group_to_source (cr);
 * cairo_mask (cr, mask);
 * cairo_pattern_destroy (mask);
 * </programlisting></informalexample>
 *
 * Returns: A new cairo_patten_t to be used as the mask.
 **/
cairo_pattern_t *
swfdec_movie_mask (cairo_t *cr, SwfdecMovie *movie, const SwfdecRect *inval)
{
  SwfdecColorTransform black;

  swfdec_color_transform_init_mask (&black);
  cairo_push_group_with_content (cr, CAIRO_CONTENT_ALPHA);
  swfdec_movie_render (movie, cr, &black, inval);
  return cairo_pop_group (cr);
}

void
swfdec_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *color_transform, const SwfdecRect *inval)
{
  SwfdecMovieClass *klass;
  SwfdecColorTransform trans;
  SwfdecRect rect;
  gboolean group;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (cr != NULL);
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
    g_warning ("%s", cairo_status_to_string (cairo_status (cr)));
  }
  g_return_if_fail (color_transform != NULL);
  g_return_if_fail (inval != NULL);
  
  if (movie->mask_of != NULL && !swfdec_color_transform_is_mask (color_transform)) {
    SWFDEC_LOG ("not rendering %s %p, movie is a mask",
	G_OBJECT_TYPE_NAME (movie), movie->name);
    return;
  }
  if (!swfdec_rect_intersect (NULL, &movie->extents, inval)) {
    SWFDEC_LOG ("not rendering %s %s, extents %g %g  %g %g are not in invalid area %g %g  %g %g",
	G_OBJECT_TYPE_NAME (movie), movie->name, 
	movie->extents.x0, movie->extents.y0, movie->extents.x1, movie->extents.y1,
	inval->x0, inval->y0, inval->x1, inval->y1);
    return;
  }
  if (!movie->visible) {
    SWFDEC_LOG ("not rendering %s %p, movie is invisible",
	G_OBJECT_TYPE_NAME (movie), movie->name);
    return;
  }

  cairo_save (cr);
  if (movie->masked_by != NULL) {
    cairo_push_group (cr);
  }
  group = swfdec_movie_needs_group (movie);
  if (group) {
    SWFDEC_DEBUG ("pushing group for blend mode %u", movie->blend_mode);
    cairo_push_group (cr);
  }

  SWFDEC_LOG ("transforming movie, transform: %g %g  %g %g   %g %g",
      movie->matrix.xx, movie->matrix.yy,
      movie->matrix.xy, movie->matrix.yx,
      movie->matrix.x0, movie->matrix.y0);
  cairo_transform (cr, &movie->matrix);
  swfdec_rect_transform (&rect, inval, &movie->inverse_matrix);
  SWFDEC_LOG ("%sinvalid area is now: %g %g  %g %g",  movie->parent ? "  " : "",
      rect.x0, rect.y0, rect.x1, rect.y1);
  swfdec_color_transform_chain (&trans, &movie->original_ctrans, color_transform);
  swfdec_color_transform_chain (&trans, &movie->color_transform, &trans);

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  g_return_if_fail (klass->render);
  klass->render (movie, cr, &trans, &rect);
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
    g_warning ("error rendering with cairo: %s", cairo_status_to_string (cairo_status (cr)));
  }
  if (group) {
    cairo_pattern_t *pattern;

    pattern = cairo_pop_group (cr);
    cairo_set_source (cr, pattern);
    cairo_set_operator (cr, swfdec_movie_get_operator_for_blend_mode (movie->blend_mode));
    cairo_paint (cr);
    cairo_pattern_destroy (pattern);
  }
  if (movie->masked_by) {
    cairo_pattern_t *mask;
    if (movie->parent == movie->masked_by->parent) {
      cairo_transform (cr, &movie->inverse_matrix);
      rect = *inval;
    } else {
      cairo_matrix_t mat, mat2;
      swfdec_movie_local_to_global_matrix (movie, &mat);
      swfdec_movie_global_to_local_matrix (movie->masked_by, &mat2);
      cairo_matrix_multiply (&mat, &mat2, &mat);
      cairo_transform (cr, &mat);
      if (cairo_matrix_invert (&mat) == CAIRO_STATUS_SUCCESS && FALSE) {
	swfdec_rect_transform (&rect, &rect, &mat);
      } else {
	SWFDEC_INFO ("non-invertible matrix when computing invalid area");
	rect.x0 = rect.y0 = -G_MAXDOUBLE;
	rect.x1 = rect.y1 = G_MAXDOUBLE;
      }
    }
    mask = swfdec_movie_mask (cr, movie->masked_by, &rect);
    cairo_pop_group_to_source (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_mask (cr, mask);
    cairo_pattern_destroy (mask);
  }
  cairo_restore (cr);
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

  switch (param_id) {
    case PROP_DEPTH:
      movie->depth = g_value_get_int (value);
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
swfdec_movie_mark (SwfdecAsObject *object)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);
  GList *walk;
  GSList *iter;

  if (movie->parent)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (movie->parent));
  swfdec_as_string_mark (movie->original_name);
  swfdec_as_string_mark (movie->name);
  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_as_object_mark (walk->data);
  }
  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    SwfdecMovieVariableListener *listener = iter->data;
    swfdec_as_object_mark (listener->object);
    swfdec_as_string_mark (listener->name);
  }
  swfdec_as_object_mark (SWFDEC_AS_OBJECT (movie->resource));

  SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->mark (object);
}

/* FIXME: This function can definitely be implemented easier */
SwfdecMovie *
swfdec_movie_get_by_name (SwfdecMovie *movie, const char *name, gboolean unnamed)
{
  GList *walk;
  int i;
  gulong l;
  guint version = SWFDEC_AS_OBJECT (movie)->context->version;
  char *end;
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);

  if ((version >= 7 && g_str_has_prefix (name, "_level")) ||
      (version < 7 && strncasecmp (name, "_level", 6) == 0)) {
    errno = 0;
    l = strtoul (name + 6, &end, 10);
    if (errno != 0 || *end != 0 || l > G_MAXINT)
      return NULL;
    i = l - 16384;
    for (walk = player->priv->roots; walk; walk = walk->next) {
      SwfdecMovie *cur = walk->data;
      if (cur->depth < i)
	continue;
      if (cur->depth == i)
	return cur;
      break;
    }
  }

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->original_name == SWFDEC_AS_STR_EMPTY && !unnamed)
      continue;
    if (swfdec_strcmp (version, cur->name, name) == 0)
      return cur;
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

static gboolean
swfdec_movie_get_variable (SwfdecAsObject *object, SwfdecAsObject *orig,
    const char *variable, SwfdecAsValue *val, guint *flags)
{
  SwfdecMovie *movie, *ret;
  
  movie = SWFDEC_MOVIE (object);
  movie = swfdec_movie_resolve (movie);
  if (movie == NULL)
    return FALSE;
  object = SWFDEC_AS_OBJECT (movie);

  if (SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->get (object, orig, variable, val, flags))
    return TRUE;

  if (swfdec_movie_get_asprop (movie, variable, val)) {
    *flags = 0;
    return TRUE;
  }

  /* FIXME: check that this is correct */
  if (object->context->version > 5 && variable == SWFDEC_AS_STR__global) {
    SWFDEC_AS_VALUE_SET_OBJECT (val, SWFDEC_AS_OBJECT (movie->resource->sandbox));
    *flags = 0;
    return TRUE;
  }
  
  ret = swfdec_movie_get_by_name (movie, variable, FALSE);
  if (ret) {
    if ((!SWFDEC_IS_ACTOR (ret) && !SWFDEC_IS_VIDEO_MOVIE (ret)) ||
	(swfdec_movie_get_version (movie) <= 5 && SWFDEC_IS_TEXT_FIELD_MOVIE (ret)))
      ret = movie;
    SWFDEC_AS_VALUE_SET_OBJECT (val, SWFDEC_AS_OBJECT (ret));
    *flags = 0;
    return TRUE;
  }
  return FALSE;
}

void
swfdec_movie_add_variable_listener (SwfdecMovie *movie, SwfdecAsObject *object,
    const char *name, const SwfdecMovieVariableListenerFunction function)
{
  SwfdecMovieVariableListener *listener;
  GSList *iter;

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    listener = iter->data;

    if (listener->object == object && listener->name == name &&
	listener->function == function)
      break;
  }
  if (iter != NULL)
    return;

  listener = g_new0 (SwfdecMovieVariableListener, 1);
  listener->object = object;
  listener->name = name;
  listener->function = function;

  movie->variable_listeners = g_slist_prepend (movie->variable_listeners,
      listener);
}

void
swfdec_movie_remove_variable_listener (SwfdecMovie *movie,
    SwfdecAsObject *object, const char *name,
    const SwfdecMovieVariableListenerFunction function)
{
  GSList *iter;

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    SwfdecMovieVariableListener *listener = iter->data;

    if (listener->object == object && listener->name == name &&
	listener->function == function)
      break;
  }
  if (iter == NULL)
    return;

  g_free (iter->data);
  movie->variable_listeners =
    g_slist_remove (movie->variable_listeners, iter->data);
}

static void
swfdec_movie_call_variable_listeners (SwfdecMovie *movie, const char *name,
    const SwfdecAsValue *val)
{
  GSList *iter;

  for (iter = movie->variable_listeners; iter != NULL; iter = iter->next) {
    SwfdecMovieVariableListener *listener = iter->data;

    if (listener->name != name &&
	(SWFDEC_AS_OBJECT (movie)->context->version >= 7 ||
	 !swfdec_str_case_equal (listener->name, name)))
      continue;

    listener->function (listener->object, name, val);
  }
}

static void
swfdec_movie_set_variable (SwfdecAsObject *object, const char *variable, 
    const SwfdecAsValue *val, guint flags)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);

  movie = swfdec_movie_resolve (movie);
  if (movie == NULL)
    return;
  object = SWFDEC_AS_OBJECT (movie);

  if (swfdec_movie_set_asprop (movie, variable, val))
    return;

  swfdec_movie_call_variable_listeners (movie, variable, val);

  SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->set (object, variable, val, flags);
}

static gboolean
swfdec_movie_foreach_variable (SwfdecAsObject *object, SwfdecAsVariableForeach func, gpointer data)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);
  SwfdecAsValue val;
  GList *walk;
  gboolean ret;

  ret = SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->foreach (object, func, data);

  for (walk = movie->list; walk && ret; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->original_name == SWFDEC_AS_STR_EMPTY)
      continue;
    SWFDEC_AS_VALUE_SET_OBJECT (&val, walk->data);
    ret &= func (object, cur->name, &val, 0, data);
  }

  return ret;
}

static char *
swfdec_movie_get_debug (SwfdecAsObject *object)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);

  return swfdec_movie_get_path (movie, TRUE);
}

typedef struct {
  SwfdecMovie *		movie;
  int			depth;
} ClipEntry;

static void
swfdec_movie_do_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *ctrans, const SwfdecRect *inval)
{
  GList *g;
  GSList *walk;
  GSList *clips = NULL;
  ClipEntry *clip = NULL;

  /* exeute the movie's drawing commands */
  for (walk = movie->draws; walk; walk = walk->next) {
    SwfdecDraw *draw = walk->data;

    if (!swfdec_rect_intersect (NULL, &draw->extents, inval))
      continue;
    
    swfdec_draw_paint (draw, cr, ctrans);
  }

  /* if the movie loaded an image, draw it here now */
  if (movie->image) {
    SwfdecRenderer *renderer = swfdec_renderer_get (cr);
    cairo_surface_t *surface = swfdec_image_create_surface_transformed (movie->image,
	renderer, ctrans);
    if (surface) {
      static const cairo_matrix_t matrix = { 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 0, 0, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 0, 0 };
      cairo_pattern_t *pattern = cairo_pattern_create_for_surface (surface);
      SWFDEC_LOG ("rendering loaded image");
      cairo_pattern_set_matrix (pattern, &matrix);
      cairo_set_source (cr, pattern);
      cairo_paint (cr);
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
      mask = swfdec_movie_mask (cr, clip->movie, inval);
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
    swfdec_movie_render (child, cr, ctrans, inval);
  }
  while (clip) {
    cairo_pattern_t *mask;
    SWFDEC_INFO ("unsetting clip depth %d", clip->depth);
    mask = swfdec_movie_mask (cr, clip->movie, inval);
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
  swfdec_player_invalidate (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), &rect);

  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_movie_invalidate (walk->data, matrix, last);
  }
}

static void
swfdec_movie_class_init (SwfdecMovieClass * movie_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (movie_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (movie_class);

  object_class->dispose = swfdec_movie_dispose;
  object_class->get_property = swfdec_movie_get_property;
  object_class->set_property = swfdec_movie_set_property;

  asobject_class->mark = swfdec_movie_mark;
  asobject_class->get = swfdec_movie_get_variable;
  asobject_class->set = swfdec_movie_set_variable;
  asobject_class->foreach = swfdec_movie_foreach_variable;
  asobject_class->debug = swfdec_movie_get_debug;

  g_object_class_install_property (object_class, PROP_DEPTH,
      g_param_spec_int ("depth", "depth", "z order inside the parent",
	  G_MININT, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  movie_class->render = swfdec_movie_do_render;
  movie_class->invalidate = swfdec_movie_do_invalidate;
  movie_class->contains = swfdec_movie_do_contains;
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
    SwfdecPlayerPrivate *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context)->priv;
    player->roots = g_list_sort (player->roots, swfdec_movie_compare_depths);
  }
  g_object_notify (G_OBJECT (movie), "depth");
}

static void
swfdec_movie_set_version (SwfdecMovie *movie)
{
  SwfdecAsObject *o;
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  if (movie->parent != NULL)
    return;

  o = SWFDEC_AS_OBJECT (movie);
  cx = o->context;
  SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, SWFDEC_PLAYER (cx)->priv->system->version));
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR__version, &val);
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
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (parent == NULL || SWFDEC_IS_MOVIE (parent), NULL);
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (resource), NULL);
  g_return_val_if_fail (graphic == NULL || SWFDEC_IS_GRAPHIC (graphic), NULL);

  /* create the right movie */
  if (graphic == NULL) {
    movie = g_object_new (SWFDEC_TYPE_SPRITE_MOVIE, "depth", depth, NULL);
    size = sizeof (SwfdecSpriteMovie);
  } else {
    SwfdecGraphicClass *klass = SWFDEC_GRAPHIC_GET_CLASS (graphic);
    g_return_val_if_fail (klass->create_movie != NULL, NULL);
    movie = klass->create_movie (graphic, &size);
    movie->graphic = g_object_ref (graphic);
    movie->depth = depth;
  }
  /* register it to the VM */
  /* FIXME: It'd be nice if we'd not overuse memory here when calling this function from a script */
  if (!swfdec_as_context_use_mem (SWFDEC_AS_CONTEXT (player), size)) {
    size = 0;
  }
  g_object_ref (movie);
  /* set essential properties */
  movie->parent = parent;
  movie->resource = resource;
  if (parent) {
    parent->list = g_list_insert_sorted (parent->list, movie, swfdec_movie_compare_depths);
    SWFDEC_DEBUG ("inserting %s %s (depth %d) into %s %p", G_OBJECT_TYPE_NAME (movie), movie->name,
	movie->depth,  G_OBJECT_TYPE_NAME (parent), parent);
    /* invalidate the parent, so it gets visible */
    swfdec_movie_queue_update (parent, SWFDEC_MOVIE_INVALID_CHILDREN);
  } else {
    player->priv->roots = g_list_insert_sorted (player->priv->roots, movie, swfdec_movie_compare_depths);
  }
  /* set its name */
  if (name) {
    movie->original_name = name;
    movie->name = name;
  } else {
    movie->original_name = SWFDEC_AS_STR_EMPTY;
    if (SWFDEC_IS_SPRITE_MOVIE (movie) || SWFDEC_IS_BUTTON_MOVIE (movie)) {
      movie->name = swfdec_as_context_give_string (SWFDEC_AS_CONTEXT (player), 
	  g_strdup_printf ("instance%u", ++player->priv->unnamed_count));
    } else {
      movie->name = SWFDEC_AS_STR_EMPTY;
    }
  }
  /* add the movie to the global movies list */
  /* NB: adding to the movies list happens before setting the parent.
   * Setting the parent does a gotoAndPlay(0) for Sprites which can cause
   * new movies to be created (and added to this list)
   */
  if (SWFDEC_IS_ACTOR (movie))
    player->priv->actors = g_list_prepend (player->priv->actors, movie);
  /* only add the movie here, because it needs to be setup for the debugger */
  swfdec_as_object_add (SWFDEC_AS_OBJECT (movie), SWFDEC_AS_CONTEXT (player), size);
  swfdec_movie_set_version (movie);
  /* only setup here, the resource assumes it can access the player via the movie */
  if (resource->movie == NULL) {
    g_assert (SWFDEC_IS_SPRITE_MOVIE (movie));
    resource->movie = SWFDEC_SPRITE_MOVIE (movie);
  }
  /* the movie is invalid already */
  player->priv->invalid_pending = g_slist_prepend (player->priv->invalid_pending, movie);

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
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->original_transform = *transform;
    movie->matrix.x0 = movie->original_transform.x0;
    movie->matrix.y0 = movie->original_transform.y0;
    movie->xscale = swfdec_matrix_get_xscale (&movie->original_transform);
    movie->yscale = swfdec_matrix_get_yscale (&movie->original_transform);
    movie->rotation = swfdec_matrix_get_rotation (&movie->original_transform);
  }
  if (ctrans) {
    swfdec_movie_invalidate_last (movie);
    movie->original_ctrans = *ctrans;
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
  copy = swfdec_movie_new (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), depth, 
      parent, movie->resource, movie->graphic, name);
  if (copy == NULL)
    return NULL;
  swfdec_movie_set_static_properties (copy, &movie->original_transform,
      &movie->original_ctrans, movie->original_ratio, movie->clip_depth, 
      movie->blend_mode, SWFDEC_IS_ACTOR (movie) ? SWFDEC_ACTOR (movie)->events : NULL);
  sandbox = SWFDEC_SANDBOX (SWFDEC_AS_OBJECT (movie)->context->global);
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
      g_string_prepend (s, movie->name);
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

  if (SWFDEC_MOVIE (movie->resource->movie) != movie)
    return NULL;

  return movie->resource;
}

