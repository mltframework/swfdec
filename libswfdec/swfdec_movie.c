/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_as_strings.h"
#include "swfdec_button_movie.h"
#include "swfdec_debug.h"
#include "swfdec_debugger.h"
#include "swfdec_event.h"
#include "swfdec_graphic.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_instance.h"

/*** MOVIE ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecMovie, swfdec_movie, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_movie_init (SwfdecMovie * movie)
{
  movie->xscale = 100;
  movie->yscale = 100;
  cairo_matrix_init_identity (&movie->original_transform);
  cairo_matrix_init_identity (&movie->matrix);
  cairo_matrix_init_identity (&movie->inverse_matrix);

  swfdec_color_transform_init_identity (&movie->color_transform);
  swfdec_color_transform_init_identity (&movie->original_ctrans);

  movie->visible = TRUE;

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
    swfdec_rect_transform (&rect, &rect, &movie->matrix);
  }
  swfdec_player_invalidate (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), &rect);
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

  if (movie->cache_state < SWFDEC_MOVIE_INVALID_EXTENTS &&
      state >= SWFDEC_MOVIE_INVALID_EXTENTS)
    swfdec_movie_invalidate (movie);
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
  d = movie->rotation - swfdec_matrix_get_rotation (&movie->original_transform);
  cairo_matrix_rotate (&movie->matrix, d * G_PI / 180);
  swfdec_matrix_ensure_invertible (&movie->matrix, &movie->inverse_matrix);

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

SwfdecMovie *
swfdec_movie_find (SwfdecMovie *movie, int depth)
{
  GList *walk;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *movie = walk->data;

    if (movie->depth < depth)
      continue;
    if (movie->depth == depth)
      return movie;
    break;
  }
  return NULL;
}

static gboolean
swfdec_movie_do_remove (SwfdecMovie *movie)
{
  SwfdecPlayer *player;

  SWFDEC_LOG ("removing %s %s", G_OBJECT_TYPE_NAME (movie), movie->name);

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  movie->will_be_removed = TRUE;
  while (movie->list) {
    GList *walk = movie->list;
    while (walk && SWFDEC_MOVIE (walk->data)->will_be_removed)
      walk = walk->next;
    if (walk == NULL)
      break;
    swfdec_movie_remove (walk->data);
  }
  /* FIXME: all of this here or in destroy callback? */
  if (player->mouse_grab == movie)
    player->mouse_grab = NULL;
  if (player->mouse_drag == movie)
    player->mouse_drag = NULL;
  swfdec_movie_invalidate (movie);
  movie->depth = -16385 - movie->depth; /* don't ask me why... */
  if (movie->parent)
    movie->parent->list = g_list_sort (movie->parent->list, swfdec_movie_compare_depths);

  if (SWFDEC_IS_SPRITE_MOVIE (movie))
    return !swfdec_movie_queue_script (movie, SWFDEC_EVENT_UNLOAD);
  else
    return TRUE;
}

/**
 * swfdec_movie_remove:
 * @movie: #SwfdecMovie to remove
 *
 * Removes this movie from its parent. In contrast to swfdec_movie_destroy (),
 * it will definitely cause a removal from the display list, but depending on
 * movie, it might still be possible to reference it from Actionscript.
 **/
void
swfdec_movie_remove (SwfdecMovie *movie)
{
  gboolean result;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (movie->state > SWFDEC_MOVIE_STATE_RUNNING)
    return;
  result = swfdec_movie_do_remove (movie);
  movie->state = SWFDEC_MOVIE_STATE_REMOVED;
  if (result)
    swfdec_movie_destroy (movie);
}

/**
 * swfdec_movie_destroy:
 * @movie: #SwfdecMovie to destroy
 *
 * Removes this movie from its parent. After this it will no longer be present,
 * neither visually nor via ActionScript. This function will not cause an 
 * unload event. Compare with swfdec_movie_destroy ().
 **/
void
swfdec_movie_destroy (SwfdecMovie *movie)
{
  SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (movie);
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);

  g_assert (movie->state < SWFDEC_MOVIE_STATE_DESTROYED);
  if (movie->state < SWFDEC_MOVIE_STATE_REMOVED) {
    swfdec_movie_do_remove (movie);
  }
  SWFDEC_LOG ("destroying movie %s", movie->name);
  while (movie->list) {
    swfdec_movie_destroy (movie->list->data);
  }
  if (movie->parent) {
    if (SWFDEC_IS_DEBUGGER (player) &&
	g_list_find (movie->parent->list, movie)) {
      g_signal_emit_by_name (player, "movie-removed", movie);
    }
    movie->parent->list = g_list_remove (movie->parent->list, movie);
  } else {
    if (SWFDEC_IS_DEBUGGER (player) &&
	g_list_find (player->roots, movie)) {
      g_signal_emit_by_name (player, "movie-removed", movie);
    }
    player->roots = g_list_remove (player->roots, movie);
  }
  /* FIXME: figure out how to handle destruction pre-init/construct.
   * This is just a stop-gap measure to avoid dead movies in those queues */
  g_queue_remove (player->init_queue, movie);
  g_queue_remove (player->construct_queue, movie);
  swfdec_player_remove_all_actions (player, movie);
  if (klass->finish_movie)
    klass->finish_movie (movie);
  player->movies = g_list_remove (player->movies, movie);
  movie->state = SWFDEC_MOVIE_STATE_DESTROYED;
  /* unset prototype here, so we don't work in AS anymore */
  SWFDEC_AS_OBJECT (movie)->prototype = NULL;
  g_object_unref (movie);
}

/**
 * swfdec_movie_run_init:
 * @movie: a #SwfdecMovie
 *
 * Runs onClipEvent(initialize) on the given @movie.
 */
void
swfdec_movie_run_init (SwfdecMovie *movie)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  g_queue_remove (player->init_queue, movie);
  swfdec_movie_execute_script (movie, SWFDEC_EVENT_INITIALIZE);
}

/**
 * swfdec_movie_run_construct:
 * @movie: a #SwfdecMovie
 *
 * Runs the constructors for @movie. This is (in the given order) 
 * onClipEvent(construct), movie.onConstruct and the constructor registered
 * via Object.registerClass.
 **/
void
swfdec_movie_run_construct (SwfdecMovie *movie)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  g_queue_remove (player->construct_queue, movie);
  swfdec_movie_execute_script (movie, SWFDEC_EVENT_CONSTRUCT);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (movie), SWFDEC_AS_STR___constructor__, 0, NULL, NULL);
}

void
swfdec_movie_execute_script (SwfdecMovie *movie, SwfdecEventType condition)
{
  const char *name;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (condition != 0);

  if (movie->events) {
    swfdec_event_list_execute (movie->events, 
	SWFDEC_AS_OBJECT (movie), condition, 0);
  }
  name = swfdec_event_type_get_name (condition);
  if (name != NULL)
    swfdec_as_object_call (SWFDEC_AS_OBJECT (movie), name, 0, NULL, NULL);
}

static void
swfdec_movie_do_execute_script (gpointer movie, gpointer condition)
{
  swfdec_movie_execute_script (movie, GPOINTER_TO_UINT (condition));
}

/**
 * swfdec_movie_queue_script:
 * @movie: a #SwfdecMovie
 * @condition: the event that should happen
 *
 * Queues execution of all scripts associated with the given event.
 *
 * Returns: TRUE if there were any such events
 **/
gboolean
swfdec_movie_queue_script (SwfdecMovie *movie, SwfdecEventType condition)
{
  SwfdecPlayer *player;
  
  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), FALSE);
  g_return_val_if_fail (condition != 0, FALSE);

  if (movie->events) {
    if (!swfdec_event_list_has_conditions (movie->events, 
	  SWFDEC_AS_OBJECT (movie), condition, 0))
      return FALSE;
  } else {
    const char *name = swfdec_event_type_get_name (condition);
    if (name == NULL ||
	!swfdec_as_object_has_function (SWFDEC_AS_OBJECT (movie), name))
      return FALSE;
  }

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  swfdec_player_add_action (player, movie, swfdec_movie_do_execute_script, 
      GUINT_TO_POINTER (condition));
  return TRUE;
}

/**
 * swfdec_movie_set_variables:
 * @script: a #SwfdecMovie
 * @variables: variables to set on @movie in application-x-www-form-urlencoded 
 *             format
 * 
 * Verifies @variables to be encoded correctly and sets them as string 
 * properties on the given @movie.
 **/
void
swfdec_movie_set_variables (SwfdecMovie *movie, const char *variables)
{
  SwfdecAsObject *as;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (variables != NULL);

  as = SWFDEC_AS_OBJECT (movie);
  SWFDEC_DEBUG ("setting variables on %p: %s", movie, variables);
  while (TRUE) {
    char *name, *value;
    const char *asname;
    SwfdecAsValue val;

    while (*variables == '&')
      variables++;
    if (*variables == '\0')
      break;
    if (!swfdec_urldecode_one (variables, &name, &value, &variables)) {
      SWFDEC_WARNING ("variables invalid at \"%s\"", variables);
      break;
    }
    if (*variables != '\0' && *variables != '&') {
      SWFDEC_WARNING ("variables not delimited with & at \"%s\"", variables);
      g_free (name);
      g_free (value);
      break;
    }
    asname = swfdec_as_context_give_string (as->context, name);
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (as->context, value));
    g_free (value);
    swfdec_as_object_set_variable (as, asname, &val);
    SWFDEC_LOG ("Set variable \"%s\" to \"%s\"", name, value);
  }
}

/* NB: coordinates are in movie's coordiante system. Use swfdec_movie_get_mouse
 * if you have global coordinates */
static gboolean
swfdec_movie_mouse_in (SwfdecMovie *movie, double x, double y)
{
  SwfdecMovieClass *klass;

  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->mouse_in == NULL)
    return FALSE;
  return klass->mouse_in (movie, x, y);
}

void
swfdec_movie_local_to_global (SwfdecMovie *movie, double *x, double *y)
{
  do {
    cairo_matrix_transform_point (&movie->matrix, x, y);
  } while ((movie = movie->parent));
}

void
swfdec_movie_global_to_local (SwfdecMovie *movie, double *x, double *y)
{
  if (movie->parent)
    swfdec_movie_global_to_local (movie->parent, x, y);
  if (movie->cache_state >= SWFDEC_MOVIE_INVALID_MATRIX)
    swfdec_movie_update (movie);
  cairo_matrix_transform_point (&movie->inverse_matrix, x, y);
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
  *x = player->mouse_x;
  *y = player->mouse_y;
  swfdec_movie_global_to_local (movie, x, y);
}

void
swfdec_movie_send_mouse_change (SwfdecMovie *movie, gboolean release)
{
  double x, y;
  gboolean mouse_in;
  int button;
  SwfdecMovieClass *klass;

  swfdec_movie_get_mouse (movie, &x, &y);
  if (release) {
    mouse_in = FALSE;
    button = 0;
  } else {
    mouse_in = swfdec_movie_mouse_in (movie, x, y);
    button = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context)->mouse_button;
  }
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  g_assert (klass->mouse_change != NULL);
  klass->mouse_change (movie, x, y, mouse_in, button);
}

SwfdecMovie *
swfdec_movie_get_movie_at (SwfdecMovie *movie, double x, double y)
{
  GList *walk, *clip_walk;
  int clip_depth = 0;
  SwfdecMovie *ret;
  SwfdecMovieClass *klass;

  SWFDEC_LOG ("%s %p getting mouse at: %g %g", G_OBJECT_TYPE_NAME (movie), movie, x, y);
  if (!swfdec_rect_contains (&movie->extents, x, y)) {
    return NULL;
  }
  cairo_matrix_transform_point (&movie->inverse_matrix, &x, &y);

  /* first check if the movie can handle mouse events, and if it can,
   * ignore its children.
   * Dunno if that's correct */
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  if (klass->mouse_change) {
    if (swfdec_movie_mouse_in (movie, x, y))
      return movie;
    else
      return NULL;
  }
  for (walk = clip_walk = g_list_last (movie->list); walk; walk = walk->prev) {
    SwfdecMovie *child = walk->data;
    if (walk == clip_walk) {
      clip_depth = 0;
      for (clip_walk = clip_walk->prev; clip_walk; clip_walk = clip_walk->prev) {
	SwfdecMovie *clip = walk->data;
	if (clip->clip_depth) {
	  double tmpx = x, tmpy = y;
	  cairo_matrix_transform_point (&clip->inverse_matrix, &tmpx, &tmpy);
	  if (!swfdec_movie_mouse_in (clip, tmpx, tmpy)) {
	    SWFDEC_LOG ("skipping depth %d to %d due to clipping", clip->depth, clip->clip_depth);
	    clip_depth = child->clip_depth;
	  }
	  break;
	}
      }
    }
    if (child->clip_depth) {
      SWFDEC_LOG ("resetting clip depth");
      clip_depth = 0;
      continue;
    }
    if (child->depth <= clip_depth && clip_depth) {
      SWFDEC_DEBUG ("ignoring depth=%d, it's clipped (clip_depth %d)", child->depth, clip_depth);
      continue;
    }
    if (!child->visible) {
      SWFDEC_LOG ("child %s %s (depth %d) is invisible, ignoring", G_OBJECT_TYPE_NAME (movie), movie->name, movie->depth);
      continue;
    }

    ret = swfdec_movie_get_movie_at (child, x, y);
    if (ret)
      return ret;
  }
  return NULL;
}

void
swfdec_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *color_transform, const SwfdecRect *inval, gboolean fill)
{
  SwfdecMovieClass *klass;
  GList *g;
  int clip_depth = 0;
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

  for (g = movie->list; g; g = g_list_next (g)) {
    SwfdecMovie *child = g->data;

    if (child->clip_depth) {
      if (clip_depth) {
	/* FIXME: is clipping additive? */
	SWFDEC_INFO ("unsetting clip depth %d for new clip depth", clip_depth);
	cairo_restore (cr);
	clip_depth = 0;
      }
      if (fill == FALSE) {
	SWFDEC_WARNING ("clipping inside clipping not implemented");
      } else {
	/* FIXME FIXME FIXME: overlapping objects in the clip movie cause problems
	 * due to them being accumulated with CAIRO_FILL_RULE_EVEN_ODD
	 */
	SWFDEC_INFO ("clipping up to depth %d by using %p with depth %d", child->clip_depth,
	    child, child->depth);
	clip_depth = child->clip_depth;
	cairo_save (cr);
	swfdec_movie_render (child, cr, &trans, &rect, FALSE);
	cairo_clip (cr);
	continue;
      }
    }

    if (clip_depth && child->depth > clip_depth) {
      SWFDEC_INFO ("unsetting clip depth %d for depth %d", clip_depth, child->depth);
      clip_depth = 0;
      cairo_restore (cr);
    }

    SWFDEC_LOG ("rendering %p with depth %d", child, child->depth);
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
    g_warning ("error rendering with cairo: %s", cairo_status_to_string (cairo_status (cr)));
  }
  cairo_restore (cr);
}

static void
swfdec_movie_dispose (GObject *object)
{
  SwfdecMovie * movie = SWFDEC_MOVIE (object);

  g_assert (movie->list == NULL);

  SWFDEC_LOG ("disposing movie %s (depth %d)", movie->name, movie->depth);
  if (movie->swf) {
    g_object_unref (movie->swf);
    movie->swf = NULL;
  }
  if (movie->events) {
    swfdec_event_list_free (movie->events);
    movie->events = NULL;
  }

  G_OBJECT_CLASS (swfdec_movie_parent_class)->dispose (G_OBJECT (movie));
}

static void
swfdec_movie_class_mark (SwfdecAsObject *object)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);
  GList *walk;

  swfdec_as_string_mark (movie->name);
  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_as_object_mark (walk->data);
  }

  SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->mark (object);
}

/* FIXME: This function can definitely be implemented easier */
static SwfdecMovie *
swfdec_movie_get_by_name (SwfdecPlayer *player, const char *name)
{
  GList *walk;
  int i = SWFDEC_AS_CONTEXT (player)->version;
  gulong l;
  char *end;

  if ((i >= 7 && !g_str_has_prefix (name, "_level")) ||
      strncasecmp (name, "_level", 6) != 0)
    return NULL;

  errno = 0;
  l = strtoul (name + 6, &end, 10);
  if (errno != 0 || *end != 0 || l > G_MAXINT)
    return NULL;
  i = l - 16384;
  for (walk = player->roots; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->depth < i)
      continue;
    if (cur->depth == i)
      return cur;
    break;
  }
  return NULL;
}

static gboolean
swfdec_movie_class_get_variable (SwfdecAsObject *object, const char *variable, 
    SwfdecAsValue *val, guint *flags)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);

  if (movie->state == SWFDEC_MOVIE_STATE_DESTROYED)
    return FALSE;

  if (swfdec_movie_get_asprop (movie, variable, val)) {
    *flags = 0;
    return TRUE;
  }

  if (SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->get (object, variable, val, flags))
    return TRUE;

  /* FIXME: check that this is correct */
  if (object->context->version > 5 && variable == SWFDEC_AS_STR__global) {
    SWFDEC_AS_VALUE_SET_OBJECT (val, object->context->global);
    *flags = 0;
    return TRUE;
  }
  
  movie = swfdec_movie_get_by_name (SWFDEC_PLAYER (object->context), variable);
  if (movie) {
    SWFDEC_AS_VALUE_SET_OBJECT (val, SWFDEC_AS_OBJECT (movie));
    *flags = 0;
    return TRUE;
  }
  return FALSE;
}

static void
swfdec_movie_class_set_variable (SwfdecAsObject *object, const char *variable, 
    const SwfdecAsValue *val)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (object);

  if (movie->state == SWFDEC_MOVIE_STATE_DESTROYED)
    return;
  if (swfdec_movie_set_asprop (movie, variable, val))
    return;
  SWFDEC_AS_OBJECT_CLASS (swfdec_movie_parent_class)->set (object, variable, val);
}

static gboolean
swfdec_movie_iterate_end (SwfdecMovie *movie)
{
  return movie->parent == NULL || 
	 movie->state < SWFDEC_MOVIE_STATE_REMOVED;
}

static void
swfdec_movie_class_init (SwfdecMovieClass * movie_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (movie_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (movie_class);

  object_class->dispose = swfdec_movie_dispose;

  asobject_class->mark = swfdec_movie_class_mark;
  asobject_class->get = swfdec_movie_class_get_variable;
  asobject_class->set = swfdec_movie_class_set_variable;

  movie_class->iterate_end = swfdec_movie_iterate_end;
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

/**
 * swfdec_movie_new:
 * @player: a #SwfdecPlayer
 * @depth: depth of movie
 * @parent: the parent movie or %NULL to make this a root movie
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
swfdec_movie_new (SwfdecPlayer *player, int depth, SwfdecMovie *parent, SwfdecGraphic *graphic, const char *name)
{
  SwfdecMovie *movie;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (parent == NULL || SWFDEC_IS_MOVIE (parent), NULL);
  g_return_val_if_fail (graphic == NULL || SWFDEC_IS_GRAPHIC (graphic), NULL);

  /* create the right movie */
  if (graphic == NULL) {
    movie = g_object_new (SWFDEC_TYPE_SPRITE_MOVIE, NULL);
    size = sizeof (SwfdecSpriteMovie);
  } else {
    SwfdecGraphicClass *klass = SWFDEC_GRAPHIC_GET_CLASS (graphic);
    g_return_val_if_fail (klass->create_movie != NULL, NULL);
    movie = klass->create_movie (graphic, &size);
  }
  /* register it to the VM */
  /* FIXME: It'd be nice if we'd not overuse memory here when calling this function from a script */
  if (!swfdec_as_context_use_mem (SWFDEC_AS_CONTEXT (player), size)) {
    size = 0;
  }
  g_object_ref (movie);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (movie), SWFDEC_AS_CONTEXT (player), size);
  /* set essential properties */
  movie->depth = depth;
  movie->parent = parent;
  if (parent) {
    movie->swf = g_object_ref (parent->swf);
    parent->list = g_list_insert_sorted (parent->list, movie, swfdec_movie_compare_depths);
    SWFDEC_DEBUG ("inserting %s %s (depth %d) into %s %p", G_OBJECT_TYPE_NAME (movie), movie->name,
	movie->depth,  G_OBJECT_TYPE_NAME (parent), parent);
  } else {
    player->roots = g_list_insert_sorted (player->roots, movie, swfdec_movie_compare_depths);
  }
  /* set its name */
  if (name) {
    movie->original_name = name;
    movie->name = name;
  } else {
    movie->original_name = SWFDEC_AS_STR_EMPTY;
    if (SWFDEC_IS_SPRITE_MOVIE (movie) || SWFDEC_IS_BUTTON_MOVIE (movie)) {
      movie->name = swfdec_as_context_give_string (SWFDEC_AS_CONTEXT (player), 
	  g_strdup_printf ("instance%u", ++player->unnamed_count));
    } else {
      movie->name = SWFDEC_AS_STR_EMPTY;
    }
  }
  /* add the movie to the global movies list */
  /* NB: adding to the movies list happens before setting the parent.
   * Setting the parent does a gotoAndPlay(0) for Sprites which can cause
   * new movies to be created (and added to this list)
   */
  player->movies = g_list_prepend (player->movies, movie);
  /* emit the new-movie signal */
  if (SWFDEC_IS_DEBUGGER (player))
    g_signal_emit_by_name (player, "movie-added", movie);
  return movie;
}

/* FIXME: since this is only used in PlaceObject, wouldn't it be easier to just have
 * swfdec_movie_update_static_properties (movie); that's notified when any of these change
 * and let PlaceObject modify the movie directly?
 */
void
swfdec_movie_set_static_properties (SwfdecMovie *movie, const cairo_matrix_t *transform,
    const SwfdecColorTransform *ctrans, int ratio, int clip_depth, SwfdecEventList *events)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (clip_depth >= -16384 || clip_depth <= 0);
  g_return_if_fail (ratio >= -1);

  if (movie->modified) {
    SWFDEC_LOG ("%s has already been modified by scripts, ignoring updates", movie->name);
    return;
  }
  if (transform) {
    movie->original_transform = *transform;
    movie->xscale = swfdec_matrix_get_xscale (&movie->original_transform);
    movie->yscale = swfdec_matrix_get_yscale (&movie->original_transform);
    movie->rotation = swfdec_matrix_get_rotation (&movie->original_transform);
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
  }
  if (ctrans) {
    movie->original_ctrans = *ctrans;
    swfdec_movie_invalidate (movie);
  }
  if (ratio >= 0 && (guint) ratio != movie->original_ratio) {
    movie->original_ratio = ratio;
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
  }
  if (clip_depth && clip_depth != movie->clip_depth) {
    movie->clip_depth = clip_depth;
    /* FIXME: is this correct? */
    swfdec_movie_invalidate (movie->parent ? movie->parent : movie);
  }
  if (events) {
    if (movie->events)
      swfdec_event_list_free (movie->events);
    movie->events = swfdec_event_list_copy (events);
  }
}

SwfdecMovie *
swfdec_movie_new_for_content (SwfdecMovie *parent, const SwfdecContent *content)
{
  SwfdecPlayer *player;
  SwfdecMovie *movie;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (parent), NULL);
  g_return_val_if_fail (SWFDEC_IS_GRAPHIC (content->graphic), NULL);
  g_return_val_if_fail (swfdec_movie_find (parent, content->depth) == NULL, NULL);

  SWFDEC_DEBUG ("new movie for parent %p", parent);
  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (parent)->context);
  movie = swfdec_movie_new (player, content->depth, parent, content->graphic, 
      content->name ? swfdec_as_context_get_string (SWFDEC_AS_CONTEXT (player), content->name) : NULL);

  swfdec_movie_set_static_properties (movie, content->has_transform ? &content->transform : NULL,
      content->has_color_transform ? &content->color_transform : NULL, 
      content->ratio, content->clip_depth, content->events);
  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    g_queue_push_tail (player->init_queue, movie);
    g_queue_push_tail (player->construct_queue, movie);
    swfdec_movie_queue_script (movie, SWFDEC_EVENT_LOAD);
  }
  swfdec_movie_initialize (movie);

  return movie;
}

void
swfdec_movie_load (SwfdecMovie *movie, const char *url, const char *target)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (url != NULL);
  g_return_if_fail (target != NULL);

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  /* yay for the multiple uses of GetURL - one of the crappier Flash things */
  if (g_str_has_prefix (target, "_level")) {
    const char *nr = target + strlen ("_level");
    char *end;
    guint depth;

    errno = 0;
    depth = strtoul (nr, &end, 10);
    if (errno == 0 && *end == '\0') {
      if (url[0] == '\0') {
	swfdec_player_remove_level (player, depth);
      } else {
	SwfdecLoader *loader = swfdec_loader_load (movie->swf->loader, url);
	g_assert (loader);
	swfdec_player_add_level_from_loader (player, depth, loader, NULL);
	swfdec_loader_queue_parse (loader);
      }
    } else {
      SWFDEC_ERROR ("%s does not specify a valid level", target);
    }
    /* FIXME: what do we do here? Is returning correct?*/
    return;
  }
  swfdec_player_launch (player, url, target);
}

char *
swfdec_movie_get_path (SwfdecMovie *movie)
{
  GString *s;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);
  
  s = g_string_new ("");
  do {
    if (movie->parent) {
      g_string_prepend (s, movie->name);
      g_string_prepend_c (s, '.');
    } else {
      char *ret = g_strdup_printf ("_level%u%s",
	movie->depth + 16384, s->str);
      g_string_free (s, TRUE);
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
