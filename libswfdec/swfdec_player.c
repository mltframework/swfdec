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

#include <stdlib.h>
#include <liboil/liboil.h>

#include "swfdec_player_internal.h"
#include "swfdec_audio_internal.h"
#include "swfdec_debug.h"
#include "swfdec_event.h"
#include "swfdec_js.h"
#include "swfdec_loader_internal.h"
#include "swfdec_movie.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite_movie.h"

#include "swfdec_marshal.c"

/*** Actions ***/

typedef struct {
  SwfdecMovie *		movie;
  SwfdecActionFunc	func;
  gpointer		data;
} SwfdecPlayerAction;

void
swfdec_player_add_action (SwfdecPlayer *player, SwfdecMovie *movie,
    SwfdecActionFunc action_func, gpointer action_data)
{
  SwfdecPlayerAction *action;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (action_func != NULL);

  action = swfdec_ring_buffer_push (player->actions);
  if (action == NULL) {
    /* FIXME: limit number of actions to not get inf loops due to scripts? */
    swfdec_ring_buffer_set_size (player->actions,
	swfdec_ring_buffer_get_size (player->actions) + 16);
    action = swfdec_ring_buffer_push (player->actions);
    g_assert (action);
  }
  action->movie = movie;
  action->func = action_func;
  action->data = action_data;
}

void
swfdec_player_remove_all_actions (SwfdecPlayer *player, SwfdecMovie *movie)
{
  SwfdecPlayerAction *action;
  guint i;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));

  for (i = 0; i < swfdec_ring_buffer_get_n_elements (player->actions); i++) {
    action = swfdec_ring_buffer_peek_nth (player->actions, i);

    if (action->movie == movie)
      action->movie = NULL;
  }
}

static gboolean
swfdec_player_do_action (SwfdecPlayer *player)
{
  SwfdecPlayerAction *action;

  do {
    action = swfdec_ring_buffer_pop (player->actions);
    if (action == NULL)
      return FALSE;
  } while (action->movie == NULL); /* skip removed actions */

  action->func (action->movie, action->data);

  return TRUE;
}

/*** SwfdecPlayer ***/

enum {
  TRACE,
  INVALIDATE,
  ITERATE,
  HANDLE_MOUSE,
  AUDIO_ADDED,
  AUDIO_REMOVED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

enum {
  PROP_0,
  PROP_INITIALIZED,
  PROP_LATENCY,
  PROP_MOUSE_VISIBLE
};

G_DEFINE_TYPE (SwfdecPlayer, swfdec_player, G_TYPE_OBJECT)

void
swfdec_player_remove_movie (SwfdecPlayer *player, SwfdecMovie *movie)
{
  swfdec_movie_remove (movie);
  player->movies = g_list_remove (player->movies, movie);
}

static void
swfdec_player_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object);
  
  switch (param_id) {
    case PROP_INITIALIZED:
      g_value_set_boolean (value, swfdec_player_is_initialized (player));
      break;
    case PROP_LATENCY:
      g_value_set_uint (value, player->samples_latency);
      break;
    case PROP_MOUSE_VISIBLE:
      g_value_set_boolean (value, player->mouse_visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object);

  switch (param_id) {
    case PROP_LATENCY:
      player->samples_latency = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_dispose (GObject *object)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object);

  swfdec_player_stop_all_sounds (player);

  g_list_foreach (player->roots, (GFunc) swfdec_movie_destroy, NULL);
  g_list_free (player->roots);

  swfdec_js_finish_player (player);

  g_assert (swfdec_ring_buffer_pop (player->actions) == NULL);
  swfdec_ring_buffer_free (player->actions);
  g_assert (player->movies == NULL);
  g_assert (player->audio == NULL);

  G_OBJECT_CLASS (swfdec_player_parent_class)->dispose (object);
}

static void
swfdec_player_update_drag_movie (SwfdecPlayer *player)
{
  double mouse_x, mouse_y;
  double x, y;
  SwfdecMovie *movie;

  if (player->mouse_drag == NULL)
    return;

  movie = player->mouse_drag;
  g_assert (movie->cache_state == SWFDEC_MOVIE_UP_TO_DATE);
  mouse_x = player->mouse_x;
  mouse_y = player->mouse_y;
  swfdec_movie_global_to_local (movie->parent, &mouse_x, &mouse_y);
  mouse_x = CLAMP (mouse_x, player->mouse_drag_rect.x0, player->mouse_drag_rect.x1);
  mouse_y = CLAMP (mouse_y, player->mouse_drag_rect.y0, player->mouse_drag_rect.y1);
  SWFDEC_LOG ("mouse is at %g %g, orighinally (%g %g)", mouse_x, mouse_y, player->mouse_x, player->mouse_y);
  if (player->mouse_drag_center) {
    x = (movie->extents.x1 + movie->extents.x0) / 2;
    y = (movie->extents.y1 + movie->extents.y0) / 2;
  } else {
    x = movie->transform.x0;
    y = movie->transform.y0;
  }
  SWFDEC_LOG ("center is at %g %g, mouse is at %g %g", x, y, mouse_x, mouse_y);
  if (mouse_x != x || mouse_y != y) {
    movie->x += mouse_x - x;
    movie->y += mouse_y - y;
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
  }
}

/**
 * swfdec_player_set_drag_movie:
 * @player: a #SwfdecPlayer
 * @drag: the movie to be dragged by the mouse or NULL to unset
 * @center: TRUE if the center of @drag should be at the mouse pointer, FALSE if (0,0)
 *          of @drag should be at the mouse pointer.
 * @rect: NULL or the rectangle that clips the mouse position. The coordinates 
 *        are in the coordinate system of the parent of @drag.
 *
 * Sets or unsets the movie that is dragged by the mouse.
 **/
void
swfdec_player_set_drag_movie (SwfdecPlayer *player, SwfdecMovie *drag, gboolean center,
    SwfdecRect *rect)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (drag == NULL || SWFDEC_IS_MOVIE (drag));

  /* FIXME: need to do anything with old drag? */
  player->mouse_drag = drag;
  player->mouse_drag_center = center;
  if (rect) {
    player->mouse_drag_rect = *rect;
  } else {
    player->mouse_drag_rect.x0 = -G_MAXDOUBLE;
    player->mouse_drag_rect.y0 = -G_MAXDOUBLE;
    player->mouse_drag_rect.x1 = G_MAXDOUBLE;
    player->mouse_drag_rect.y1 = G_MAXDOUBLE;
  }
  SWFDEC_DEBUG ("starting drag in %g %g  %g %g", 
      player->mouse_drag_rect.x0, player->mouse_drag_rect.y0,
      player->mouse_drag_rect.x1, player->mouse_drag_rect.y1);
  /* FIXME: need a way to make sure we get updated */
}

static void
swfdec_player_do_mouse_move (SwfdecPlayer *player)
{
  GList *walk;
  SwfdecMovie *mouse_grab = NULL;

  swfdec_player_update_drag_movie (player);
  for (walk = player->movies; walk; walk = walk->next) {
    swfdec_movie_queue_script (walk->data, SWFDEC_EVENT_MOUSE_MOVE);
  }
  if (player->mouse_button) {
    mouse_grab = player->mouse_grab;
  } else {
    /* if the mouse button is pressed the grab widget stays the same (I think) */
    for (walk = g_list_last (player->roots); walk; walk = walk->prev) {
      mouse_grab = swfdec_movie_get_movie_at (walk->data, player->mouse_x, player->mouse_y);
      if (mouse_grab)
	break;
    }
  }
  SWFDEC_DEBUG ("%s %p has mouse at %g %g", 
      mouse_grab ? G_OBJECT_TYPE_NAME (mouse_grab) : "---", 
      mouse_grab, player->mouse_x, player->mouse_y);
  if (player->mouse_grab && mouse_grab != player->mouse_grab)
    swfdec_movie_send_mouse_change (player->mouse_grab, TRUE);
  player->mouse_grab = mouse_grab;
  if (mouse_grab)
    swfdec_movie_send_mouse_change (mouse_grab, FALSE);
}

static void
swfdec_player_do_mouse_button (SwfdecPlayer *player)
{
  GList *walk;
  guint event;

  if (player->mouse_button) {
    event = SWFDEC_EVENT_MOUSE_DOWN;
  } else {
    event = SWFDEC_EVENT_MOUSE_UP;
  }
  for (walk = player->movies; walk; walk = walk->next) {
    swfdec_movie_queue_script (walk->data, event);
  }
  if (player->mouse_grab)
    swfdec_movie_send_mouse_change (player->mouse_grab, FALSE);
}

static void
swfdec_player_emit_signals (SwfdecPlayer *player)
{
  if (!swfdec_rect_is_empty (&player->invalid)) {
    g_signal_emit (player, signals[INVALIDATE], 0, &player->invalid);
    swfdec_rect_init_empty (&player->invalid);
  }
}

static void
swfdec_player_do_handle_mouse (SwfdecPlayer *player, 
    double x, double y, int button)
{
  swfdec_player_lock (player);
  SWFDEC_LOG ("handling mouse at %g %g %d", x, y, button);
  if (player->mouse_x != x || player->mouse_y != y) {
    player->mouse_x = x;
    player->mouse_y = y;
    swfdec_player_do_mouse_move (player);
  }
  if (player->mouse_button != button) {
    player->mouse_button = button;
    swfdec_player_do_mouse_button (player);
  }
  swfdec_player_perform_actions (player);
  swfdec_player_unlock (player);
}

static void
swfdec_player_do_iterate (SwfdecPlayer *player)
{
  GList *walk;

  swfdec_player_lock (player);
  SWFDEC_INFO ("=== START ITERATION ===");
  /* set latency so that sounds start after iteration */
  if (player->samples_latency < player->samples_this_frame)
    player->samples_latency = player->samples_this_frame;
  /* The handling of this list is rather tricky. This code assumes that no 
   * movies get removed that haven't been iterated yet. This should not be a 
   * problem without using Javascript, because the only way to remove movies
   * is when a sprite removes a child. But all children are in front of their
   * parent in this list, since they got added later.
   */
  for (walk = player->movies; walk; walk = walk->next) {
    SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (walk->data);
    if (klass->iterate_start)
      klass->iterate_start (walk->data);
  }
  swfdec_player_perform_actions (player);
  SWFDEC_INFO ("=== STOP ITERATION ===");
  /* this loop allows removal of walk->data */
  walk = player->movies;
  while (walk) {
    SwfdecMovie *cur = walk->data;
    SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (cur);
    walk = walk->next;
    g_assert (klass->iterate_end);
    if (!klass->iterate_end (cur))
      swfdec_movie_destroy (cur);
  }
  /* iterate audio after video so audio clips that get added during mouse
   * events have the same behaviour than those added while iterating */
  swfdec_player_iterate_audio (player);
  swfdec_player_unlock (player);
}

void
swfdec_player_perform_actions (SwfdecPlayer *player)
{
  GList *walk;
  SwfdecRect old_inval;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  swfdec_rect_init_empty (&old_inval);
  do {
    while (swfdec_player_do_action (player));
    for (walk = player->roots; walk; walk = walk->next) {
      swfdec_movie_update (walk->data);
    }
    /* update the state of the mouse when stuff below it moved */
    if (swfdec_rect_contains (&player->invalid, player->mouse_x, player->mouse_y)) {
      SWFDEC_INFO ("=== NEED TO UPDATE mouse post-iteration ===");
      swfdec_player_do_mouse_move (player);
    }
    swfdec_rect_union (&old_inval, &old_inval, &player->invalid);
    swfdec_rect_init_empty (&player->invalid);
  } while (swfdec_ring_buffer_get_n_elements (player->actions) > 0);
  player->invalid = old_inval;
}

void
swfdec_player_lock (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_assert (swfdec_ring_buffer_get_n_elements (player->actions) == 0);
  g_assert (swfdec_rect_is_empty (&player->invalid));

  g_object_freeze_notify (G_OBJECT (player));
}

void
swfdec_player_unlock (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_assert (swfdec_ring_buffer_get_n_elements (player->actions) == 0);

  g_object_thaw_notify (G_OBJECT (player));
  swfdec_player_emit_signals (player);
}

static void
swfdec_player_class_init (SwfdecPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = swfdec_player_get_property;
  object_class->set_property = swfdec_player_set_property;
  object_class->dispose = swfdec_player_dispose;

  g_object_class_install_property (object_class, PROP_INITIALIZED,
      g_param_spec_boolean ("initialized", "initialized", "the player has initialized its basic values",
	  FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_LATENCY,
      g_param_spec_uint ("latency", "latency", "audio latency in samples",
	  0, 44100 * 10, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MOUSE_VISIBLE,
      g_param_spec_boolean ("mouse-visible", "mouse visible", "whether to show the mouse pointer",
	  TRUE, G_PARAM_READABLE));

  signals[TRACE] = g_signal_new ("trace", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
      G_TYPE_NONE, 1, G_TYPE_STRING);
  signals[INVALIDATE] = g_signal_new ("invalidate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
      G_TYPE_NONE, 1, G_TYPE_POINTER);
  signals[ITERATE] = g_signal_new ("iterate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SwfdecPlayerClass, iterate), 
      NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
  signals[HANDLE_MOUSE] = g_signal_new ("handle-mouse", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SwfdecPlayerClass, handle_mouse), 
      NULL, NULL, swfdec_marshal_VOID__DOUBLE_DOUBLE_INT,
      G_TYPE_NONE, 3, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INT);
  signals[AUDIO_ADDED] = g_signal_new ("audio-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, SWFDEC_TYPE_AUDIO);
  signals[AUDIO_REMOVED] = g_signal_new ("audio-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, SWFDEC_TYPE_AUDIO);

  klass->iterate = swfdec_player_do_iterate;
  klass->handle_mouse = swfdec_player_do_handle_mouse;
}

static void
swfdec_player_init (SwfdecPlayer *player)
{
  swfdec_js_init_player (player);

  player->actions = swfdec_ring_buffer_new_for_type (SwfdecPlayerAction, 16);

  player->mouse_visible = TRUE;
}

void
swfdec_player_stop_all_sounds (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  while (player->audio) {
    swfdec_audio_remove (player->audio->data);
  }
}

void
swfdec_player_invalidate (SwfdecPlayer *player, const SwfdecRect *rect)
{
  if (swfdec_rect_is_empty (rect)) {
    g_assert_not_reached ();
    return;
  }

  swfdec_rect_union (&player->invalid, &player->invalid, rect);
  SWFDEC_DEBUG ("toplevel invalidation of %g %g  %g %g - invalid region now %g %g  %g %g",
      rect->x0, rect->y0, rect->x1, rect->y1,
      player->invalid.x0, player->invalid.y0, player->invalid.x1, player->invalid.y1);
}

SwfdecRootMovie *
swfdec_player_add_level_from_loader (SwfdecPlayer *player, guint depth,
    SwfdecLoader *loader)
{
  SwfdecMovie *movie;
  SwfdecRootMovie *root;
  GList *found;

  movie = swfdec_movie_new_for_player (player, depth);
  root = SWFDEC_ROOT_MOVIE (movie);
  root->loader = loader;
  root->loader->target = root;
  root->player = player;
  found = g_list_find_custom (player->roots, movie, swfdec_movie_compare_depths);
  if (found) {
    SWFDEC_DEBUG ("remove existing movie _level%u", depth);
    swfdec_movie_remove (found->data);
    player->roots = g_list_delete_link (player->roots, found);
  }
  player->roots = g_list_insert_sorted (player->roots, movie, swfdec_movie_compare_depths);
  return root;
}

void
swfdec_player_remove_level (SwfdecPlayer *player, guint depth)
{
  GList *walk;

  for (walk = player->roots; walk; walk = walk->next) {
    SwfdecMovie *movie = walk->data;

    if (movie->depth < depth)
      continue;

    if (movie->depth == depth) {
      SWFDEC_DEBUG ("remove existing movie _level%u", depth);
      swfdec_movie_remove (movie);
      player->roots = g_list_delete_link (player->roots, walk);
      return;
    }
    break;
  }
  SWFDEC_LOG ("no movie to remove at level %u", depth);
}

/** PUBLIC API ***/

/**
 * swfdec_player_new:
 *
 * Creates a new player.
 * This function calls swfdec_init () for you if it wasn't called before.
 *
 * Returns: The new player
 **/
SwfdecPlayer *
swfdec_player_new (void)
{
  SwfdecPlayer *player;

  swfdec_init ();
  player = g_object_new (SWFDEC_TYPE_PLAYER, NULL);

  return player;
}

/**
 * swfdec_player_set_loader:
 * @player: a #SwfdecPlayer
 * @loader: the loader to use for this player. Takes ownership of the given loader.
 *
 * Sets the loader for the main data. This function only works if no loader has 
 * been set on @player yet.
 * <note>If you want to capture events during the setup process, you want to 
 * connect your signal handlers before calling swfdec_player_set_loader() and
 * not use conveniencse functions such as swfdec_player_new_from_file().</note>
 **/
void
swfdec_player_set_loader (SwfdecPlayer *player, SwfdecLoader *loader)
{
  SwfdecRootMovie *movie;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (player->roots == NULL);
  g_return_if_fail (SWFDEC_IS_LOADER (loader));

  movie = swfdec_player_add_level_from_loader (player, 0, loader);
  swfdec_root_movie_parse (movie);
}

/**
 * swfdec_player_new_from_file:
 * @filename: name of the file to play
 * @error: return location for error or NULL
 *
 * Tries to create a player to play back the given file. If the file does not
 * exist or another error occurs, NULL is returned.
 * This function calls swfdec_init () for you if it wasn't called before.
 *
 * Returns: a new player or NULL on error.
 **/
SwfdecPlayer *
swfdec_player_new_from_file (const char *filename, GError **error)
{
  SwfdecLoader *loader;
  SwfdecPlayer *player;

  g_return_val_if_fail (filename != NULL, NULL);

  loader = swfdec_loader_new_from_file (filename, error);
  if (loader == NULL)
    return NULL;
  player = swfdec_player_new ();
  swfdec_player_set_loader (player, loader);

  return player;
}

/**
 * swfdec_init:
 *
 * Initializes the Swfdec library.
 **/
void
swfdec_init (void)
{
  static gboolean _inited = FALSE;
  const char *s;

  if (_inited)
    return;

  _inited = TRUE;

  g_type_init ();
  oil_init ();

  s = g_getenv ("SWFDEC_DEBUG");
  if (s && s[0]) {
    char *end;
    int level;

    level = strtoul (s, &end, 0);
    if (end[0] == 0) {
      swfdec_debug_set_level (level);
    }
  }
  swfdec_js_init (0);
}

/**
 * swfdec_player_handle_mouse:
 * @dec: a #SwfdecPlayer
 * @x: x coordinate of mouse
 * @y: y coordinate of mouse
 * @button: 1 for pressed, 0 for not pressed
 *
 * Updates the current mouse status. If the mouse has left the area of @player,
 * you should pass values outside the movie size for @x and @y.
 **/
void
swfdec_player_handle_mouse (SwfdecPlayer *player, 
    double x, double y, int button)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (button == 0 || button == 1);

  g_signal_emit (player, signals[HANDLE_MOUSE], 0, x, y, button);
}

/**
 * swfdec_player_render:
 * @dec: a #SwfdecPlayer
 * @cr: #cairo_t to render to
 * @area: #SwfdecRect describing the area to render or NULL for whole area
 *
 * Renders the given @area of the current frame to @cr.
 **/
void
swfdec_player_render (SwfdecPlayer *player, cairo_t *cr, SwfdecRect *area)
{
  static const SwfdecColorTransform trans = { 256, 0, 256, 0, 256, 0, 256, 0 };
  GList *walk;
  SwfdecRect full = { 0, 0, player->width, player->height };

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (cr != NULL);

  /* FIXME: fail when !initialized? */
  if (!swfdec_player_is_initialized (player))
    return;

  if (area == NULL)
    area = &full;
  if (swfdec_rect_is_empty (area))
    return;
  SWFDEC_INFO ("=== %p: START RENDER, area %g %g  %g %g ===", player, 
      area->x0, area->y0, area->x1, area->y1);
  cairo_rectangle (cr, area->x0, area->y0, area->x1 - area->x0, area->y1 - area->y0);
  cairo_clip (cr);
  /* FIXME: find a nicer way to render the background */
  if (player->roots == NULL ||
      !SWFDEC_IS_SPRITE_MOVIE (player->roots->data) ||
      !swfdec_sprite_movie_paint_background (SWFDEC_SPRITE_MOVIE (player->roots->data), cr)) {
    SWFDEC_INFO ("couldn't paint the background, using white");
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_paint (cr);
  }

  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_render (walk->data, cr, &trans, area, TRUE);
  }
  SWFDEC_INFO ("=== %p: END RENDER ===", player);
}

/**
 * swfdec_player_iterate:
 * @player: the #SwfdecPlayer to iterate
 *
 * Advances #player to the next frame. You should make sure to call this function
 * as often per second as swfdec_player_get_rate() indicates.
 **/
void
swfdec_player_iterate (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

#if 0
  while (TRUE)
    swfdec_js_run (player, "i = new Object(); i.foo = 7", NULL);
  //swfdec_js_run (player, "s=\"/A/B:foo\"; t=s.indexOf (\":\"); if (t) t=s.substring(0,s.indexOf (\":\")); else t=s;", NULL);
#endif

  g_signal_emit (player, signals[ITERATE], 0);
}

/**
 * swfdec_player_is_initialized:
 * @player: a #SwfdecPlayer
 *
 * Determines if the @player is initalized yet. An initialized player is able
 * to provide basic values like width, height or rate. A player may not be 
 * initialized if the loader it was started with does not reference a Flash
 * resources or it did not provide enough data yet. If a player is initialized,
 * it will never be uninitialized again.
 *
 * Returns: TRUE if the basic values are known.
 **/
gboolean
swfdec_player_is_initialized (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);

  return player->width > 0 && player->height > 0;
}

/**
 * swfdec_player_get_rate:
 * @player: a #SwfdecPlayer
 *
 * Queries the framerate of this movie. This number specifies the number
 * of frames that are supposed to pass per second. It is a 
 * multiple of 1/256.
 *
 * Returns: The framerate of this movie or 0 if it isn't known yet.
 **/
double
swfdec_player_get_rate (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0.0);

  return player->rate / 256.0;
}

/**
 * swfdec_player_get_image_size:
 * @player: a #SwfdecPlayer
 * @width: integer to store the width in or NULL
 * @height: integer to store the height in or NULL
 *
 * If the size of the movie is already known, fills in @width and @height with
 * the size. Otherwise @width and @height are set to 0.
 **/
void
swfdec_player_get_image_size (SwfdecPlayer *player, int *width, int *height)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  if (width)
    *width = player->width;
  if (height)
    *height = player->height;
}

/* FIXME: I don't like this function */
const GList *
swfdec_player_get_audio (SwfdecPlayer *	player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  return player->audio;
}

