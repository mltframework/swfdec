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
#include "swfdec_sprite.h"

/*** MOVIE ***/

static const SwfdecContent default_content = {
  NULL, -1, 0, 0, { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
  { 256, 0, 256, 0, 256, 0, 256, 0 },
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
  swfdec_color_transform_init_identity (&movie->color_transform);

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
 * Queues an update of all cached values inside @movie and invalidates it.
 **/
void
swfdec_movie_queue_update (SwfdecMovie *movie, SwfdecMovieState state)
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
  cairo_matrix_rotate (mat, movie->rotation * G_PI / 180);
  cairo_matrix_multiply (mat, mat, &movie->content->transform);
  movie->inverse_transform = *mat;
  if (cairo_matrix_invert (&movie->inverse_transform)) {
    /* the matrix is somehow weird, carefully adjust it */
    mat = &movie->inverse_transform;
    *mat = movie->content->transform;
    if (cairo_matrix_invert (mat)) {
      g_assert_not_reached ();
    }
    cairo_matrix_rotate (mat, -movie->rotation * G_PI / 180);
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
 * Sets new contents for @movie. Note that name and graphic of @content must 
 * be identical to the current content of @movie.
 **/
void
swfdec_movie_set_content (SwfdecMovie *movie, const SwfdecContent *content)
{
  const SwfdecContent *old_content;
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (content == NULL) {
    content = &default_content;
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

  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

SwfdecMovie *
swfdec_movie_find (SwfdecMovie *movie, guint depth)
{
  GList *walk;

  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), NULL);

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *movie = walk->data;

    if (movie->content->depth < depth)
      continue;
    if (movie->content->depth == depth)
      return movie;
    break;
  }
  return NULL;
}

typedef void (* SwfdecMovieRemoveFunc) (SwfdecMovie *, gpointer);

static void
swfdec_movie_do_remove (SwfdecMovie *movie, gpointer child_remove)
{
  /* remove all children */
  while (movie->list) {
    (*(SwfdecMovieRemoveFunc) child_remove) (movie->list->data, child_remove);
  }
  if (SWFDEC_ROOT_MOVIE (movie->root)->player->mouse_grab == movie)
    SWFDEC_ROOT_MOVIE (movie->root)->player->mouse_grab = NULL;
  if (SWFDEC_ROOT_MOVIE (movie->root)->player->mouse_drag == movie)
    SWFDEC_ROOT_MOVIE (movie->root)->player->mouse_drag = NULL;
  swfdec_movie_invalidate (movie);
  if (movie->parent)
    movie->parent->list = g_list_remove (movie->parent->list, movie);
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
  SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (movie->root)->player;

  SWFDEC_LOG ("destroying movie %s", movie->name);
  swfdec_movie_do_remove (movie, swfdec_movie_destroy);
  swfdec_movie_set_content (movie, NULL);
  if (klass->finish_movie)
    klass->finish_movie (movie);
  if (movie->parent) {
    movie->parent->list = g_list_remove (movie->parent->list, movie);
  }
  if (movie->jsobj)
    swfdec_js_movie_remove_property (movie);
  player->movies = g_list_remove (player->movies, movie);
  g_object_unref (movie);
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
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  SWFDEC_LOG ("removing %s %s", G_OBJECT_TYPE_NAME (movie), movie->name);
  swfdec_movie_do_remove (movie, swfdec_movie_remove);
  if (!swfdec_movie_queue_script (movie, SWFDEC_EVENT_UNLOAD))
    swfdec_movie_destroy (movie);
}

static void
swfdec_movie_execute_script (SwfdecMovie *movie, gpointer data)
{
  guint condition = GPOINTER_TO_UINT (data);

  g_assert (movie->content->events);
  swfdec_event_list_execute (movie->content->events, movie, condition, 0);
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

  if (movie->content->events == NULL)
    return FALSE;
  if (!swfdec_event_list_has_conditions (movie->content->events, condition, 0))
    return FALSE;

  player = SWFDEC_ROOT_MOVIE (movie->root)->player;
  swfdec_player_add_action (player, movie, swfdec_movie_execute_script, 
      GUINT_TO_POINTER (condition));
  return TRUE;
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
    cairo_matrix_transform_point (&movie->transform, x, y);
  } while ((movie = movie->parent));
}

void
swfdec_movie_global_to_local (SwfdecMovie *movie, double *x, double *y)
{
  if (movie->parent)
    swfdec_movie_global_to_local (movie->parent, x, y);
  cairo_matrix_transform_point (&movie->inverse_transform, x, y);
}

void
swfdec_movie_get_mouse (SwfdecMovie *movie, double *x, double *y)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  player = SWFDEC_ROOT_MOVIE (movie->root)->player;
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
    button = SWFDEC_ROOT_MOVIE (movie->root)->player->mouse_button;
  }
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  g_assert (klass->mouse_change != NULL);
  klass->mouse_change (movie, x, y, mouse_in, button);
}

SwfdecMovie *
swfdec_movie_get_movie_at (SwfdecMovie *movie, double x, double y)
{
  GList *walk, *clip_walk;
  unsigned int clip_depth;
  SwfdecMovie *ret;
  SwfdecMovieClass *klass;

  SWFDEC_LOG ("%s %p getting mouse at: %g %g", G_OBJECT_TYPE_NAME (movie), movie, x, y);
  if (!swfdec_rect_contains (&movie->extents, x, y)) {
    return NULL;
  }
  cairo_matrix_transform_point (&movie->inverse_transform, &x, &y);

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
	if (clip->content->clip_depth) {
	  double tmpx = x, tmpy = y;
	  cairo_matrix_transform_point (&clip->inverse_transform, &tmpx, &tmpy);
	  if (!swfdec_movie_mouse_in (clip, tmpx, tmpy)) {
	    SWFDEC_LOG ("skipping depth %u to %u due to clipping", clip->content->depth, clip->content->clip_depth);
	    clip_depth = child->content->clip_depth;
	  }
	  break;
	}
      }
    }
    if (child->content->clip_depth) {
      SWFDEC_LOG ("resetting clip depth");
      clip_depth = 0;
      continue;
    }
    if (child->content->depth <= clip_depth && clip_depth) {
      SWFDEC_DEBUG ("ignoring depth=%d, it's clipped (clip_depth %u)", child->content->depth, clip_depth);
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
  swfdec_color_transform_chain (&trans, &movie->color_transform, &trans);

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
	/* FIXME FIXME FIXME: overlapping objects in the clip movie cause problems
	 * due to them being accumulated with CAIRO_FILL_RULE_EVEN_ODD
	 */
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

  SWFDEC_LOG ("disposing movie %s", movie->name);
  g_free (movie->name);
  if (movie->parent)
    g_object_unref (movie->parent);

  G_OBJECT_CLASS (swfdec_movie_parent_class)->dispose (G_OBJECT (movie));
}

static gboolean
swfdec_movie_iterate_end (SwfdecMovie *movie)
{
  return movie->parent == NULL || 
	 g_list_find (movie->parent->list, movie) != NULL;
}

static void
swfdec_movie_class_init (SwfdecMovieClass * movie_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (movie_class);

  object_class->dispose = swfdec_movie_dispose;

  movie_class->iterate_end = swfdec_movie_iterate_end;
}

static void
swfdec_movie_set_name (SwfdecMovie *movie)
{
  /* FIXME: implement this function in a smarter way, not if (IS_FOO_MOVIE (x)) */
  g_assert (movie->name == NULL);
  if (movie->content->name) {
    movie->name = g_strdup (movie->content->name);
    swfdec_js_movie_add_property (movie);
    movie->has_name = TRUE;
  } else if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    /* FIXME: figure out if it's relative to root or player or something else
     * entirely 
     */
    SwfdecRootMovie *root = SWFDEC_ROOT_MOVIE (movie->root);
    movie->name = g_strdup_printf ("instance%u", ++root->unnamed_count);
    movie->has_name = FALSE;
  } else {
    movie->name = g_strdup (G_OBJECT_TYPE_NAME (movie));
    movie->has_name = FALSE;
  }
  SWFDEC_LOG ("created movie %s", movie->name);
}

static void
swfdec_movie_set_parent (SwfdecMovie *movie)
{
  SwfdecMovie *parent = movie->parent;
  SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (movie->root)->player;
  SwfdecMovieClass *klass;

  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  if (parent) {
    parent->list = g_list_insert_sorted (parent->list, movie, swfdec_movie_compare_depths);
    SWFDEC_DEBUG ("inserting %s %p (depth %u) into %s %p", G_OBJECT_TYPE_NAME (movie), movie,
	movie->content->depth,  G_OBJECT_TYPE_NAME (parent), parent);
  }
  swfdec_movie_set_name (movie);
  klass = SWFDEC_MOVIE_GET_CLASS (movie);
  /* NB: adding to the movies list happens before setting the parent.
   * Setting the parent does a gotoAndPlay(0) for Sprites which can cause
   * new movies to be created (and added to this list)
   */
  player->movies = g_list_prepend (player->movies, movie);
  if (klass->init_movie)
    klass->init_movie (movie);
  swfdec_movie_queue_script (movie, SWFDEC_EVENT_LOAD);
}

static void
swfdec_movie_initialize (SwfdecMovie *movie, const SwfdecContent *content)
{
  const SwfdecContent *old;

  old = movie->content;
  movie->content = content;
  swfdec_movie_set_parent (movie);
  movie->content = old;
  swfdec_movie_set_content (movie, content);
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

  g_return_val_if_fail (SWFDEC_IS_MOVIE (parent), NULL);
  g_return_val_if_fail (SWFDEC_IS_GRAPHIC (content->graphic), NULL);

  SWFDEC_DEBUG ("new movie for parent %p", parent);
  klass = SWFDEC_GRAPHIC_GET_CLASS (content->graphic);
  g_return_val_if_fail (klass->create_movie != NULL, NULL);
  ret = klass->create_movie (content->graphic);
  ret->parent = parent;
  g_object_ref (parent);
  ret->root = parent->root;
  swfdec_movie_initialize (ret, content);
  return ret;
}

SwfdecMovie *
swfdec_movie_new_for_player (SwfdecPlayer *player, guint depth)
{
  SwfdecMovie *ret;
  SwfdecContent *content;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  content = swfdec_content_new (depth);
  cairo_matrix_scale (&content->transform, 1 / SWFDEC_SCALE_FACTOR, 1 / SWFDEC_SCALE_FACTOR);
  content->name = g_strdup_printf ("_level%u", depth);
  ret = g_object_new (SWFDEC_TYPE_ROOT_MOVIE, NULL);
  g_object_weak_ref (G_OBJECT (ret), (GWeakNotify) swfdec_content_free, content);
  SWFDEC_ROOT_MOVIE (ret)->player = player;
  ret->root = ret;
  swfdec_movie_initialize (ret, content);

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

