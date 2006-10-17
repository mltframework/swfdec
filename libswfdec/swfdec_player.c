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
#include "swfdec_audio.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_loader_internal.h"
#include "swfdec_movie.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite_movie.h"

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
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

enum {
  PROP_0,
  PROP_INITIALIZED,
  PROP_INVALID,
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
      /* FIXME: what about rate? FLV has no rate... */
      g_value_set_boolean (value, player->width > 0 && player->height > 0);
    case PROP_INVALID:
      g_value_set_boolean (value, swfdec_rect_is_empty (&player->invalid));
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
  guint i;

  while (player->movies) {
    swfdec_player_remove_movie (player, player->movies->data);
  }

  swfdec_js_finish_player (player);

  g_assert (swfdec_ring_buffer_pop (player->actions) == NULL);
  swfdec_ring_buffer_free (player->actions);
  g_assert (player->movies == NULL);

  for (i = 0; i < player->audio->len; i++) {
    swfdec_audio_finish (&g_array_index (player->audio, SwfdecAudio, i));
  }
  g_array_free (player->audio, TRUE);

  G_OBJECT_CLASS (swfdec_player_parent_class)->dispose (object);
}

static void
swfdec_player_class_init (SwfdecPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = swfdec_player_get_property;
  object_class->set_property = swfdec_player_set_property;
  object_class->dispose = swfdec_player_dispose;

  g_object_class_install_property (object_class, PROP_INVALID,
      g_param_spec_boolean ("initialized", "initialized", "the player has initialized its basic values",
	  FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_INVALID,
      g_param_spec_boolean ("invalid", "invalid", "player contains invalid areas",
	  TRUE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_LATENCY,
      g_param_spec_uint ("latency", "latency", "audio latency in samples",
	  0, 44100 * 10, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MOUSE_VISIBLE,
      g_param_spec_boolean ("mouse-visible", "mouse visible", "whether to show the mouse pointer",
	  TRUE, G_PARAM_READABLE));

  signals[TRACE] = g_signal_new ("trace", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
      G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
swfdec_player_init (SwfdecPlayer *player)
{
  swfdec_js_init_player (player);

  player->audio = g_array_new (FALSE, FALSE, sizeof (SwfdecAudio));
  player->actions = swfdec_ring_buffer_new_for_type (SwfdecPlayerAction, 16);
}

void
swfdec_player_invalidate (SwfdecPlayer *player, const SwfdecRect *rect)
{
  gboolean was_empty;

  if (swfdec_rect_is_empty (rect))
    return;

  was_empty = swfdec_rect_is_empty (&player->invalid);
  swfdec_rect_union (&player->invalid, &player->invalid, rect);
  if (was_empty)
    g_object_notify (G_OBJECT (player), "invalid");
}

static void
swfdec_player_add_level_from_loader (SwfdecPlayer *player, guint depth,
    SwfdecLoader *loader)
{
  SwfdecMovie *movie;
  SwfdecRootMovie *root;
  GList *found;

  movie = g_object_new (SWFDEC_TYPE_ROOT_MOVIE, NULL);
  root = SWFDEC_ROOT_MOVIE (movie);
  root->loader = loader;
  root->player = player;
  movie->root = movie;
  found = g_list_find_custom (player->roots, movie, swfdec_movie_compare_depths);
  if (found) {
    SWFDEC_DEBUG ("remove existing movie _level%u", depth);
    swfdec_movie_remove (found->data);
    player->roots = g_list_delete_link (player->roots, found);
  }
  player->roots = g_list_insert_sorted (player->roots, movie, swfdec_movie_compare_depths);
  swfdec_root_movie_parse (root);
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
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (player->roots == NULL);
  g_return_if_fail (SWFDEC_IS_LOADER (loader));

  swfdec_player_add_level_from_loader (player, 0, loader);
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
 * swfdec_player_get_invalid:
 * @player: a #SwfdecPlayer
 * @rect: pointer to a rectangle to be filled with the invalid area
 *
 * The player accumulates the parts that need a redraw. This function gets
 * the rectangle that encloses these parts. Use swfdec_clear_invalid() to clear
 * it.
 **/
void
swfdec_player_get_invalid (SwfdecPlayer *player, SwfdecRect *rect)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (rect != NULL);

  *rect = player->invalid;
}

/**
 * swfdec_player_clear_invalid:
 * @player: a #SwfdecPlayer
 *
 * Clears the list of areas that need a redraw.
 **/
void
swfdec_player_clear_invalid (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  if (!swfdec_rect_is_empty (&player->invalid)) {
    swfdec_rect_init_empty (&player->invalid);
    g_object_notify (G_OBJECT (player), "invalid");
  }
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
  GList *walk;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (button == 0 || button == 1);

  SWFDEC_LOG ("handling mouse at %g %g %d", x, y, button);
  g_object_freeze_notify (G_OBJECT (player));
  for (walk = g_list_last (player->roots); walk; walk = walk->prev) {
    if (swfdec_movie_handle_mouse (walk->data, x, y, button))
      break;
  }
  while (swfdec_player_do_action (player));
  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_update (walk->data);
  }
  g_object_thaw_notify (G_OBJECT (player));
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
  static const SwfdecColorTransform trans = { { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 } };
  gboolean was_empty;
  GList *walk;
  SwfdecRect full = { 0, 0, player->width, player->height };

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (cr != NULL);

  if (area == NULL)
    area = &full;
  if (swfdec_rect_is_empty (area))
    return;
  was_empty = swfdec_rect_is_empty (&player->invalid);
  SWFDEC_INFO ("=== %p: START RENDER, area %g %g  %g %g ===", player, 
      area->x0, area->y0, area->x1, area->y1);
  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_render (walk->data, cr, &trans, area, TRUE);
  }
  SWFDEC_INFO ("=== %p: END RENDER ===", player);
  swfdec_rect_subtract (&player->invalid, &player->invalid, area);
  if (!was_empty && swfdec_rect_is_empty (&player->invalid))
    g_object_notify (G_OBJECT (player), "invalid");
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
  GList *walk;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

#if 0
  swfdec_js_run (dec, "foo = __swfdec_target;", NULL);
#endif

  g_object_freeze_notify (G_OBJECT (player));
  SWFDEC_INFO ("=== START ITERATION ===");
  /* iterate audio before video so we don't iterate audio clips that get added this frame */
  swfdec_player_iterate_audio (player);
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
  while (swfdec_player_do_action (player));
  for (walk = player->movies; walk; walk = walk->next) {
    SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (walk->data);
    if (klass->iterate_end)
      klass->iterate_end (walk->data);
  }
  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_update (walk->data);
  }
  SWFDEC_INFO ("=== STOP ITERATION ===");
  g_object_thaw_notify (G_OBJECT (player));
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

