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
#include <string.h>
#include <stdlib.h>
#include <liboil/liboil.h>

#include "swfdec_player_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_internal.h"
#include "swfdec_button_movie.h" /* for mouse cursor */
#include "swfdec_cache.h"
#include "swfdec_debug.h"
#include "swfdec_enums.h"
#include "swfdec_event.h"
#include "swfdec_initialize.h"
#include "swfdec_internal.h"
#include "swfdec_loader_internal.h"
#include "swfdec_marshal.h"
#include "swfdec_movie.h"
#include "swfdec_script.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_instance.h"

/*** gtk-doc ***/

/**
 * SECTION:SwfdecPlayer
 * @title: SwfdecPlayer
 * @short_description: main playback object
 *
 * A #SwfdecPlayer is the main object used for playing back Flash files through
 * Swfdec.
 *
 * A player interacts with the outside world in a multitude of ways. The most 
 * important ones are described below.
 *
 * Input is handled via the 
 * <link linkend="swfdec-SwfdecLoader">SwfdecLoader</link> class. A 
 * #SwfdecLoader is set on a new player using swfdec_player_set_loader().
 *
 * When the loader has provided enough data, you can start playing the file.
 * This is done in steps by calling swfdec_player_advance() - preferrably as 
 * often as swfdec_player_get_next_event() indicates. Or you can provide user input
 * to the player by calling for example swfdec_player_handle_mouse().
 *
 * You can use swfdec_player_render() to draw the current state of the player.
 * After that, connect to the SwfdecPlayer::invalidate signal to be notified of
 * changes.
 *
 * Audio output is handled via the 
 * <link linkend="swfdec-SwfdecAudio">SwfdecAudio</link> class. One 
 * #SwfdecAudio object is created for every output using the 
 * SwfdecPlayer::audio-added signal.
 */

/**
 * SwfdecPlayer:
 *
 * This is the base object used for playing Flash files.
 */

/**
 * SECTION:Enumerations
 * @title: Enumerations
 * @short_description: enumerations used in Swfdec
 *
 * This file lists all of the enumerations used in various parts of Swfdec.
 */

/**
 * SwfdecMouseCursor:
 * @SWFDEC_MOUSE_CURSOR_NORMAL: a normal mouse cursor
 * @SWFDEC_MOUSE_CURSOR_NONE: no mouse image
 * @SWFDEC_MOUSE_CURSOR_TEXT: a mouse cursor suitable for text editing
 * @SWFDEC_MOUSE_CURSOR_CLICK: a mouse cursor for clicking a hyperlink or a 
 *                             button
 *
 * This enumeration describes the possible types for the SwfdecPlayer::mouse-cursor
 * property.
 */

/**
 * SwfdecAlignment:
 * @SWFDEC_ALIGNMENT_TOP_LEFT: top left
 * @SWFDEC_ALIGNMENT_TOP: top
 * @SWFDEC_ALIGNMENT_TOP_RIGHT: top right
 * @SWFDEC_ALIGNMENT_LEFT: left
 * @SWFDEC_ALIGNMENT_CENTER: center
 * @SWFDEC_ALIGNMENT_RIGHT: right
 * @SWFDEC_ALIGNMENT_BOTTOM_LEFT: left
 * @SWFDEC_ALIGNMENT_BOTTOM: bottom
 * @SWFDEC_ALIGNMENT_BOTTOM_RIGHT: bottom right
 *
 * These are the possible values for the alignment of an unscaled movie.
 */

/**
 * SwfdecScaleMode:
 * @SWFDEC_SCALE_SHOW_ALL: Show the whole content as large as possible
 * @SWFDEC_SCALE_NO_BORDER: Fill the whole area, possibly cropping parts
 * @SWFDEC_SCALE_EXACT_FIT: Fill the whole area, don't keep aspect ratio
 * @SWFDEC_SCALE_NONE: Do not scale the movie at all
 *
 * Describes how the movie should be scaled if the given size doesn't equal the
 * movie's size.
 */

/*** Timeouts ***/

static SwfdecTick
swfdec_player_get_next_event_time (SwfdecPlayer *player)
{
  if (player->timeouts) {
    return ((SwfdecTimeout *) player->timeouts->data)->timestamp - player->time;
  } else {
    return 0;
  }
}

/**
 * swfdec_player_add_timeout:
 * @player: a #SwfdecPlayer
 * @timeout: timeout to add
 *
 * Adds a timeout to @player. The timeout will be removed automatically when 
 * triggered, so you need to use swfdec_player_add_timeout() to add it again. 
 * The #SwfdecTimeout struct and callback does not use a data callback pointer. 
 * It's suggested that you use the struct as part of your own bigger struct 
 * and get it back like this:
 * <programlisting>
 * typedef struct {
 *   // ...
 *   SwfdecTimeout timeout;
 * } MyStruct;
 *
 * static void
 * my_struct_timeout_callback (SwfdecTimeout *timeout)
 * {
 *   MyStruct *mystruct = (MyStruct *) ((void *) timeout - G_STRUCT_OFFSET (MyStruct, timeout));
 *
 *   // do stuff
 * }
 * </programlisting>
 **/
void
swfdec_player_add_timeout (SwfdecPlayer *player, SwfdecTimeout *timeout)
{
  GList *walk;
  SwfdecTick next_tick;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (timeout != NULL);
  g_return_if_fail (timeout->timestamp > player->time);
  g_return_if_fail (timeout->callback != NULL);

  SWFDEC_LOG ("adding timeout %p", timeout);
  next_tick = swfdec_player_get_next_event_time (player);
  /* the order is important, on events with the same time, we make sure the new one is last */
  for (walk = player->timeouts; walk; walk = walk->next) {
    SwfdecTimeout *cur = walk->data;
    if (cur->timestamp > timeout->timestamp)
      break;
  }
  player->timeouts = g_list_insert_before (player->timeouts, walk, timeout);
  if (next_tick != swfdec_player_get_next_event_time (player))
    g_object_notify (G_OBJECT (player), "next-event");
}

/**
 * swfdec_player_remove_timeout:
 * @player: a #SwfdecPlayer
 * @timeout: a timeout that should be removed
 *
 * Removes the @timeout from the list of scheduled timeouts. The timeout must 
 * have been added with swfdec_player_add_timeout() before.
 **/
void
swfdec_player_remove_timeout (SwfdecPlayer *player, SwfdecTimeout *timeout)
{
  SwfdecTick next_tick;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (timeout != NULL);
  g_return_if_fail (timeout->timestamp > player->time);
  g_return_if_fail (timeout->callback != NULL);

  SWFDEC_LOG ("removing timeout %p", timeout);
  next_tick = swfdec_player_get_next_event_time (player);
  player->timeouts = g_list_remove (player->timeouts, timeout);
  if (next_tick != swfdec_player_get_next_event_time (player))
    g_object_notify (G_OBJECT (player), "next-event");
}

/*** Actions ***/

typedef struct {
  gpointer		object;
  SwfdecActionFunc	func;
  gpointer		data;
} SwfdecPlayerAction;

/**
 * swfdec_player_add_action:
 * @player: a #SwfdecPlayer
 * @object: object identifying the action
 * @action_func: function to execute
 * @action_data: additional data to pass to @func
 *
 * Adds an action to the @player. Actions are used by Flash player to solve
 * reentrancy issues. Instead of calling back into the Actionscript engine,
 * an action is queued for later execution. So if you're writing code that
 * is calling Actionscript code, you want to do this by using actions.
 **/
void
swfdec_player_add_action (SwfdecPlayer *player, gpointer object,
    SwfdecActionFunc action_func, gpointer action_data)
{
  SwfdecPlayerAction *action;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (object != NULL);
  g_return_if_fail (action_func != NULL);

  SWFDEC_LOG ("adding action %p %p %p", object, action_func, action_data);
  action = swfdec_ring_buffer_push (player->actions);
  if (action == NULL) {
    /* FIXME: limit number of actions to not get inf loops due to scripts? */
    swfdec_ring_buffer_set_size (player->actions,
	swfdec_ring_buffer_get_size (player->actions) + 16);
    action = swfdec_ring_buffer_push (player->actions);
    g_assert (action);
  }
  action->object = object;
  action->func = action_func;
  action->data = action_data;
}

/**
 * swfdec_player_remove_all_actions:
 * @player: a #SwfdecPlayer
 * @object: object pointer identifying the actions to be removed
 *
 * Removes all actions associated with @object. See swfdec_player_add_action()
 * for details about actions.
 **/
void
swfdec_player_remove_all_actions (SwfdecPlayer *player, gpointer object)
{
  SwfdecPlayerAction *action;
  guint i;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (object != NULL);

  for (i = 0; i < swfdec_ring_buffer_get_n_elements (player->actions); i++) {
    action = swfdec_ring_buffer_peek_nth (player->actions, i);

    if (action->object == object) {
      SWFDEC_LOG ("removing action %p %p %p", 
	  action->object, action->func, action->data);
      action->object = NULL;
    }
  }
}

static gboolean
swfdec_player_do_action (SwfdecPlayer *player)
{
  SwfdecPlayerAction *action;
  SwfdecMovie *movie;

  movie = g_queue_peek_head (player->init_queue);
  if (movie) {
    swfdec_movie_run_init (movie);
    return TRUE;
  }
  movie = g_queue_peek_head (player->construct_queue);
  if (movie) {
    swfdec_movie_run_construct (movie);
    return TRUE;
  }
  do {
    action = swfdec_ring_buffer_pop (player->actions);
    if (action == NULL)
      return FALSE;
  } while (action->object == NULL); /* skip removed actions */

  action->func (action->object, action->data);
  SWFDEC_LOG ("executing action %p %p %p", 
      action->object, action->func, action->data);

  return TRUE;
}

static void
swfdec_player_perform_external_actions (SwfdecPlayer *player)
{
  SwfdecPlayerAction *action;
  guint i;

  /* we need to query the number of current actions so newly added ones aren't
   * executed in here */
  for (i = swfdec_ring_buffer_get_n_elements (player->external_actions); i > 0; i--) {
    action = swfdec_ring_buffer_pop (player->external_actions);
    g_assert (action != NULL);
    /* skip removed actions */
    if (action->object == NULL) 
      continue;
    action->func (action->object, action->data);
  }

  if (player->external_timeout.callback) {
    swfdec_player_remove_timeout (player, &player->external_timeout);
    player->external_timeout.callback = NULL;
  }
}

static void
swfdec_player_trigger_external_actions (SwfdecTimeout *advance)
{
  SwfdecPlayer *player = SWFDEC_PLAYER ((guint8 *) advance - G_STRUCT_OFFSET (SwfdecPlayer, external_timeout));

  player->external_timeout.callback = NULL;
  swfdec_player_perform_external_actions (player);
}

void
swfdec_player_add_external_action (SwfdecPlayer *player, gpointer object, 
    SwfdecActionFunc action_func, gpointer action_data)
{
  SwfdecPlayerAction *action;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (object != NULL);
  g_return_if_fail (action_func != NULL);

  SWFDEC_LOG ("adding external action %p %p %p", object, action_func, action_data);
  action = swfdec_ring_buffer_push (player->external_actions);
  if (action == NULL) {
    /* FIXME: limit number of actions to not get inf loops due to scripts? */
    swfdec_ring_buffer_set_size (player->actions,
	swfdec_ring_buffer_get_size (player->actions) + 16);
    action = swfdec_ring_buffer_push (player->actions);
    g_assert (action);
  }
  action->object = object;
  action->func = action_func;
  action->data = action_data;
  if (!player->external_timeout.callback) {
    /* trigger execution in 100 ms */
    player->external_timeout.timestamp = player->time + SWFDEC_MSECS_TO_TICKS (100);
    player->external_timeout.callback = swfdec_player_trigger_external_actions;
    swfdec_player_add_timeout (player, &player->external_timeout);
  }
}

void
swfdec_player_remove_all_external_actions (SwfdecPlayer *player, gpointer object)
{
  SwfdecPlayerAction *action;
  guint i;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (object != NULL);

  for (i = 0; i < swfdec_ring_buffer_get_n_elements (player->external_actions); i++) {
    action = swfdec_ring_buffer_peek_nth (player->external_actions, i);

    if (action->object == object) {
      SWFDEC_LOG ("removing external action %p %p %p", 
	  action->object, action->func, action->data);
      action->object = NULL;
    }
  }
}

/*** SwfdecPlayer ***/

enum {
  INVALIDATE,
  ADVANCE,
  HANDLE_MOUSE,
  AUDIO_ADDED,
  AUDIO_REMOVED,
  LAUNCH,
  FSCOMMAND,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

enum {
  PROP_0,
  PROP_CACHE_SIZE,
  PROP_INITIALIZED,
  PROP_MOUSE_CURSOR,
  PROP_NEXT_EVENT,
  PROP_BACKGROUND_COLOR,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_ALIGNMENT,
  PROP_SCALE
};

G_DEFINE_TYPE (SwfdecPlayer, swfdec_player, SWFDEC_TYPE_AS_CONTEXT)

void
swfdec_player_remove_movie (SwfdecPlayer *player, SwfdecMovie *movie)
{
  swfdec_movie_remove (movie);
  player->movies = g_list_remove (player->movies, movie);
}

static guint
swfdec_player_alignment_to_flags (SwfdecAlignment alignment)
{
  static const guint align_flags[9] = { 
    SWFDEC_ALIGN_FLAG_TOP | SWFDEC_ALIGN_FLAG_LEFT,
    SWFDEC_ALIGN_FLAG_TOP,
    SWFDEC_ALIGN_FLAG_TOP | SWFDEC_ALIGN_FLAG_RIGHT,
    SWFDEC_ALIGN_FLAG_LEFT,
    0,
    SWFDEC_ALIGN_FLAG_RIGHT,
    SWFDEC_ALIGN_FLAG_BOTTOM | SWFDEC_ALIGN_FLAG_LEFT,
    SWFDEC_ALIGN_FLAG_BOTTOM,
    SWFDEC_ALIGN_FLAG_BOTTOM | SWFDEC_ALIGN_FLAG_RIGHT
  };
  return align_flags[alignment];
}

static SwfdecAlignment
swfdec_player_alignment_from_flags (guint flags)
{
  if (flags & SWFDEC_ALIGN_FLAG_TOP) {
    if (flags & SWFDEC_ALIGN_FLAG_LEFT) {
      return SWFDEC_ALIGNMENT_TOP_LEFT;
    } else if (flags & SWFDEC_ALIGN_FLAG_RIGHT) {
      return SWFDEC_ALIGNMENT_TOP_RIGHT;
    } else {
      return SWFDEC_ALIGNMENT_TOP;
    }
  } else if (flags & SWFDEC_ALIGN_FLAG_BOTTOM) {
    if (flags & SWFDEC_ALIGN_FLAG_LEFT) {
      return SWFDEC_ALIGNMENT_BOTTOM_LEFT;
    } else if (flags & SWFDEC_ALIGN_FLAG_RIGHT) {
      return SWFDEC_ALIGNMENT_BOTTOM_RIGHT;
    } else {
      return SWFDEC_ALIGNMENT_BOTTOM;
    }
  } else {
    if (flags & SWFDEC_ALIGN_FLAG_LEFT) {
      return SWFDEC_ALIGNMENT_LEFT;
    } else if (flags & SWFDEC_ALIGN_FLAG_RIGHT) {
      return SWFDEC_ALIGNMENT_RIGHT;
    } else {
      return SWFDEC_ALIGNMENT_CENTER;
    }
  }
}

static void
swfdec_player_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object);
  
  switch (param_id) {
    case PROP_BACKGROUND_COLOR:
      g_value_set_uint (value, swfdec_player_get_background_color (player));
      break;
    case PROP_CACHE_SIZE:
      g_value_set_uint (value, player->cache->max_size);
      break;
    case PROP_INITIALIZED:
      g_value_set_boolean (value, swfdec_player_is_initialized (player));
      break;
    case PROP_MOUSE_CURSOR:
      g_value_set_enum (value, player->mouse_cursor);
      break;
    case PROP_NEXT_EVENT:
      g_value_set_uint (value, swfdec_player_get_next_event (player));
      break;
    case PROP_WIDTH:
      g_value_set_int (value, player->stage_width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, player->stage_height);
      break;
    case PROP_ALIGNMENT:
      g_value_set_enum (value, swfdec_player_alignment_from_flags (player->align_flags));
      break;
    case PROP_SCALE:
      g_value_set_enum (value, player->scale_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_update_scale (SwfdecPlayer *player)
{
  int width, height;
  double scale_x, scale_y;

  width = player->stage_width >= 0 ? player->stage_width : (int) player->width;
  height = player->stage_height >= 0 ? player->stage_height : (int) player->height;
  if (height == 0 || width == 0) {
    player->scale_x = 1.0;
    player->scale_y = 1.0;
    player->offset_x = 0;
    player->offset_y = 0;
    return;
  }
  if (player->width == 0 || player->height == 0) {
    scale_x = 1.0;
    scale_y = 1.0;
  } else {
    scale_x = (double) width / player->width;
    scale_y = (double) height / player->height;
  }
  switch (player->scale_mode) {
    case SWFDEC_SCALE_SHOW_ALL:
      player->scale_x = MIN (scale_x, scale_y);
      player->scale_y = player->scale_x;
      break;
    case SWFDEC_SCALE_NO_BORDER:
      player->scale_x = MAX (scale_x, scale_y);
      player->scale_y = player->scale_x;
      break;
    case SWFDEC_SCALE_EXACT_FIT:
      player->scale_x = scale_x;
      player->scale_y = scale_y;
      break;
    case SWFDEC_SCALE_NONE:
      player->scale_x = 1.0;
      player->scale_y = 1.0;
      break;
    default:
      g_assert_not_reached ();
  }
  width = width - ceil (player->width * player->scale_x);
  height = height - ceil (player->height * player->scale_y);
  if (player->align_flags & SWFDEC_ALIGN_FLAG_LEFT) {
    player->offset_x = 0;
  } else if (player->align_flags & SWFDEC_ALIGN_FLAG_RIGHT) {
    player->offset_x = width;
  } else {
    player->offset_x = width / 2;
  }
  if (player->align_flags & SWFDEC_ALIGN_FLAG_TOP) {
    player->offset_y = 0;
  } else if (player->align_flags & SWFDEC_ALIGN_FLAG_BOTTOM) {
    player->offset_y = height;
  } else {
    player->offset_y = height / 2;
  }
  SWFDEC_LOG ("coordinate translation is %g * x + %d - %g * y + %d", 
      player->scale_x, player->offset_x, player->scale_y, player->offset_y);
#if 0
  /* FIXME: make this emit the signal at the right time */
  player->invalid.x0 = 0;
  player->invalid.y0 = 0;
  player->invalid.x1 = player->stage_width;
  player->invalid.y1 = player->stage_height;
#endif
}

static void
swfdec_player_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object);

  switch (param_id) {
    case PROP_BACKGROUND_COLOR:
      swfdec_player_set_background_color (player, g_value_get_uint (value));
      break;
    case PROP_CACHE_SIZE:
      player->cache->max_size = g_value_get_uint (value);
      break;
    case PROP_WIDTH:
      swfdec_player_set_size (player, g_value_get_int (value), player->stage_height);
      break;
    case PROP_HEIGHT:
      swfdec_player_set_size (player, player->stage_width, g_value_get_int (value));
      break;
    case PROP_ALIGNMENT:
      player->align_flags = swfdec_player_alignment_to_flags (g_value_get_enum (value));
      swfdec_player_update_scale (player);
      break;
    case PROP_SCALE:
      swfdec_player_set_scale_mode (player, g_value_get_enum (value));
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
  g_hash_table_destroy (player->registered_classes);

  while (player->roots)
    swfdec_movie_destroy (player->roots->data);

  swfdec_player_remove_all_actions (player, player); /* HACK to allow non-removable actions */

  /* we do this here so references to GC'd objects get freed */
  G_OBJECT_CLASS (swfdec_player_parent_class)->dispose (object);

#ifndef G_DISABLE_ASSERT
  {
    SwfdecPlayerAction *action;
    while ((action = swfdec_ring_buffer_pop (player->actions)) != NULL) {
      g_assert (action->object == NULL); /* skip removed actions */
    }
  }
#endif
  g_assert (swfdec_ring_buffer_pop (player->actions) == NULL);
  swfdec_ring_buffer_free (player->actions);
  g_assert (player->movies == NULL);
  g_assert (player->audio == NULL);
  if (player->rate) {
    swfdec_player_remove_timeout (player, &player->iterate_timeout);
  }
  g_assert (player->timeouts == NULL);
  g_list_free (player->intervals);
  player->intervals = NULL;
  g_assert (g_queue_is_empty (player->init_queue));
  g_assert (g_queue_is_empty (player->construct_queue));
  g_queue_free (player->init_queue);
  g_queue_free (player->construct_queue);
  swfdec_cache_unref (player->cache);
  if (player->loader) {
    g_object_unref (player->loader);
    player->loader = NULL;
  }
}

static void
swfdec_player_broadcast (SwfdecPlayer *player, const char *object_name, const char *signal)
{
  SwfdecAsValue val;
  SwfdecAsObject *obj;

  SWFDEC_DEBUG ("broadcasting message %s.%s", object_name, signal);
  obj = SWFDEC_AS_CONTEXT (player)->global;
  swfdec_as_object_get_variable (obj, object_name, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  obj = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  SWFDEC_AS_VALUE_SET_STRING (&val, signal);
  swfdec_as_object_call (obj, SWFDEC_AS_STR_broadcastMessage, 1, &val, NULL);
}

static void
swfdec_player_update_mouse_cursor (SwfdecPlayer *player)
{
  SwfdecMouseCursor new = SWFDEC_MOUSE_CURSOR_NORMAL;

  if (!player->mouse_visible) {
    new = SWFDEC_MOUSE_CURSOR_NONE;
  } else if (player->mouse_grab != NULL) {
    /* FIXME: this needs to be more sophisticated, since SwfdecEditText may
     * want to have different mouse cursors depending on location (it supports
     * links in theory)
     */
    if (SWFDEC_IS_BUTTON_MOVIE (player->mouse_grab))
      new = SWFDEC_MOUSE_CURSOR_CLICK;
  }

  if (new != player->mouse_cursor) {
    player->mouse_cursor = new;
    g_object_notify (G_OBJECT (player), "mouse-cursor");
  }
}

static void
swfdec_player_update_drag_movie (SwfdecPlayer *player)
{
  double x, y;
  SwfdecMovie *movie;

  if (player->mouse_drag == NULL)
    return;

  movie = player->mouse_drag;
  g_assert (movie->cache_state == SWFDEC_MOVIE_UP_TO_DATE);
  x = player->mouse_x;
  y = player->mouse_y;
  if (movie->parent)
    swfdec_movie_global_to_local (movie->parent, &x, &y);
  if (player->mouse_drag_center) {
    x -= (movie->extents.x1 - movie->extents.x0) / 2;
    y -= (movie->extents.y1 - movie->extents.y0) / 2;
  } else {
    x -= player->mouse_drag_x;
    y -= player->mouse_drag_y;
  }
  x = CLAMP (x, player->mouse_drag_rect.x0, player->mouse_drag_rect.x1);
  y = CLAMP (y, player->mouse_drag_rect.y0, player->mouse_drag_rect.y1);
  SWFDEC_LOG ("mouse is at %g %g, orighinally (%g %g)", x, y, player->mouse_x, player->mouse_y);
  if (x != movie->matrix.x0 || y != movie->matrix.y0) {
    movie->matrix.x0 = x;
    movie->matrix.y0 = y;
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
  if (drag && !center) {
    player->mouse_drag_x = player->mouse_x;
    player->mouse_drag_y = player->mouse_y;
    if (drag->parent)
      swfdec_movie_global_to_local (drag->parent, &player->mouse_drag_x, &player->mouse_drag_y);
    player->mouse_drag_x -= drag->matrix.x0;
    player->mouse_drag_y -= drag->matrix.y0;
  }
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
  if (drag) {
    swfdec_movie_update (drag);
    drag->modified = TRUE;
    swfdec_player_update_drag_movie (player);
  }
}

static void
swfdec_player_update_mouse_position (SwfdecPlayer *player)
{
  GList *walk;
  SwfdecMovie *mouse_grab = NULL;

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
swfdec_player_do_mouse_move (SwfdecPlayer *player)
{
  GList *walk;

  swfdec_player_update_drag_movie (player);
  for (walk = player->movies; walk; walk = walk->next) {
    swfdec_movie_queue_script (walk->data, SWFDEC_EVENT_MOUSE_MOVE);
  }
  swfdec_player_broadcast (player, SWFDEC_AS_STR_Mouse, SWFDEC_AS_STR_onMouseMove);
  swfdec_player_update_mouse_position (player);
}

static void
swfdec_player_do_mouse_button (SwfdecPlayer *player)
{
  GList *walk;
  guint event;
  const char *event_name;

  if (player->mouse_button) {
    event = SWFDEC_EVENT_MOUSE_DOWN;
    event_name = SWFDEC_AS_STR_onMouseDown;
  } else {
    event = SWFDEC_EVENT_MOUSE_UP;
    event_name = SWFDEC_AS_STR_onMouseUp;
  }
  for (walk = player->movies; walk; walk = walk->next) {
    swfdec_movie_queue_script (walk->data, event);
  }
  swfdec_player_broadcast (player, SWFDEC_AS_STR_Mouse, event_name);
  if (player->mouse_grab)
    swfdec_movie_send_mouse_change (player->mouse_grab, FALSE);
}

static void
swfdec_player_emit_signals (SwfdecPlayer *player)
{
  GList *walk;

  /* emit invalidate signal */
  if (!swfdec_rect_is_empty (&player->invalid)) {
    double x, y, width, height;
    /* FIXME: currently we clamp the rectangle to the visible area, it might
     * be useful to allow out-of-bounds drawing. In that case this needs to be
     * changed */
    x = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.x0) * player->scale_x + player->offset_x;
    x = MAX (x, 0.0);
    y = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.y0) * player->scale_y + player->offset_y;
    y = MAX (y, 0.0);
    width = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.x1 - player->invalid.x0) * player->scale_x;
    width = MIN (width, player->stage_width - x);
    height = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.y1 - player->invalid.y0) * player->scale_y;
    height = MIN (height, player->stage_height - y);
    g_signal_emit (player, signals[INVALIDATE], 0, x, y, width, height);
    swfdec_rect_init_empty (&player->invalid);
  }

  /* emit audio-added for all added audio streams */
  for (walk = player->audio; walk; walk = walk->next) {
    SwfdecAudio *audio = walk->data;

    if (audio->added)
      continue;
    g_signal_emit (player, signals[AUDIO_ADDED], 0, audio);
    audio->added = TRUE;
  }
}

static gboolean
swfdec_player_do_handle_mouse (SwfdecPlayer *player, 
    double x, double y, int button)
{
  swfdec_player_lock (player);
  x -= player->offset_x;
  y -= player->offset_y;
  x = x * SWFDEC_TWIPS_SCALE_FACTOR / player->scale_x;
  y = y * SWFDEC_TWIPS_SCALE_FACTOR / player->scale_y;
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

  /* FIXME: allow events to pass through */
  return TRUE;
}

static void
swfdec_player_iterate (SwfdecTimeout *timeout)
{
  SwfdecPlayer *player = SWFDEC_PLAYER ((guint8 *) timeout - G_STRUCT_OFFSET (SwfdecPlayer, iterate_timeout));
  GList *walk;

  SWFDEC_INFO ("=== START ITERATION ===");
  /* start the iteration. This performs a goto next frame on all 
   * movies that are not stopped. It also queues onEnterFrame.
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
  /* add timeout again */
  /* FIXME: rounding issues? */
  player->iterate_timeout.timestamp += SWFDEC_TICKS_PER_SECOND * 256 / player->rate;
  swfdec_player_add_timeout (player, &player->iterate_timeout);
}

static void
swfdec_player_do_advance (SwfdecPlayer *player, guint msecs, guint audio_samples)
{
  GList *walk;
  SwfdecAudio *audio;
  SwfdecTimeout *timeout;
  SwfdecTick target_time;
  guint frames_now;
  
  swfdec_player_lock (player);
  swfdec_player_perform_external_actions (player);
  target_time = player->time + SWFDEC_MSECS_TO_TICKS (msecs);
  SWFDEC_DEBUG ("advancing %u msecs (%u audio frames)", msecs, audio_samples);

  player->audio_skip = audio_samples;
  /* iterate all playing sounds */
  walk = player->audio;
  while (walk) {
    audio = walk->data;
    walk = walk->next;
    if (swfdec_audio_iterate (audio, audio_samples) == 0)
      swfdec_audio_remove (audio);
  }

  for (timeout = player->timeouts ? player->timeouts->data : NULL;
       timeout && timeout->timestamp <= target_time; 
       timeout = player->timeouts ? player->timeouts->data : NULL) {
    player->timeouts = g_list_remove (player->timeouts, timeout);
    frames_now = SWFDEC_TICKS_TO_SAMPLES (timeout->timestamp) -
      SWFDEC_TICKS_TO_SAMPLES (player->time);
    player->time = timeout->timestamp;
    player->audio_skip -= frames_now;
    SWFDEC_LOG ("activating timeout %p now (timeout is %"G_GUINT64_FORMAT", target time is %"G_GUINT64_FORMAT,
	timeout, timeout->timestamp, target_time);
    timeout->callback (timeout);
    swfdec_player_perform_actions (player);
  }
  if (target_time > player->time) {
    frames_now = SWFDEC_TICKS_TO_SAMPLES (target_time) -
      SWFDEC_TICKS_TO_SAMPLES (player->time);
    player->time = target_time;
    player->audio_skip -= frames_now;
  }
  g_assert (player->audio_skip == 0);
  
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
      swfdec_player_update_mouse_position (player);
      for (walk = player->roots; walk; walk = walk->next) {
	swfdec_movie_update (walk->data);
      }
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
  SWFDEC_DEBUG ("LOCKED");
}

void
swfdec_player_unlock (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_assert (swfdec_ring_buffer_get_n_elements (player->actions) == 0);

  SWFDEC_DEBUG ("UNLOCK");
  swfdec_as_context_maybe_gc (SWFDEC_AS_CONTEXT (player));
  swfdec_player_update_mouse_cursor (player);
  g_object_thaw_notify (G_OBJECT (player));
  swfdec_player_emit_signals (player);
}

static gboolean
swfdec_accumulate_or (GSignalInvocationHint *ihint, GValue *return_accu, 
    const GValue *handler_return, gpointer data)
{
  if (g_value_get_boolean (handler_return))
    g_value_set_boolean (return_accu, TRUE);
  return TRUE;
}

static void
swfdec_player_mark_string_object (gpointer key, gpointer value, gpointer data)
{
  swfdec_as_string_mark (key);
  swfdec_as_object_mark (value);
}

static void
swfdec_player_mark (SwfdecAsContext *context)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (context);
  GList *walk;

  g_hash_table_foreach (player->registered_classes, swfdec_player_mark_string_object, NULL);
  swfdec_as_object_mark (player->MovieClip);
  swfdec_as_object_mark (player->Video);
  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_as_object_mark (walk->data);
  }

  SWFDEC_AS_CONTEXT_CLASS (swfdec_player_parent_class)->mark (context);
}

static void
swfdec_player_get_time (SwfdecAsContext *context, GTimeVal *tv)
{
  *tv = context->start_time;

  /* FIXME: what granularity do we want? Currently it's milliseconds */
  g_time_val_add (tv, SWFDEC_TICKS_TO_MSECS (SWFDEC_PLAYER (context)->time) * 1000);
}

static void
swfdec_player_class_init (SwfdecPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsContextClass *context_class = SWFDEC_AS_CONTEXT_CLASS (klass);

  object_class->get_property = swfdec_player_get_property;
  object_class->set_property = swfdec_player_set_property;
  object_class->dispose = swfdec_player_dispose;

  g_object_class_install_property (object_class, PROP_INITIALIZED,
      g_param_spec_boolean ("initialized", "initialized", "TRUE when the player has initialized its basic values",
	  FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_MOUSE_CURSOR,
      g_param_spec_enum ("mouse-cursor", "mouse cursor", "how the mouse pointer should be presented",
	  SWFDEC_TYPE_MOUSE_CURSOR, SWFDEC_MOUSE_CURSOR_NONE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_NEXT_EVENT,
      g_param_spec_uint ("next-event", "next event", "how many milliseconds until the next event or 0 when no event pending",
	  0, G_MAXUINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_CACHE_SIZE,
      g_param_spec_uint ("cache-size", "cache size", "maximum cache size in bytes",
	  0, G_MAXUINT, 50 * 1024 * 1024, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_BACKGROUND_COLOR,
      g_param_spec_uint ("background-color", "background color", "ARGB color used to draw the background",
	  0, G_MAXUINT, SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0xFF), G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_WIDTH,
      g_param_spec_int ("width", "width", "current width of the movie",
	  -1, G_MAXINT, -1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HEIGHT,
      g_param_spec_int ("height", "height", "current height of the movie",
	  -1, G_MAXINT, -1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ALIGNMENT,
      g_param_spec_enum ("alignment", "alignment", "point of the screen to align the output to",
	  SWFDEC_TYPE_ALIGNMENT, SWFDEC_ALIGNMENT_CENTER, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SCALE,
      g_param_spec_enum ("scale-mode", "scale mode", "method used to scale the movie",
	  SWFDEC_TYPE_SCALE_MODE, SWFDEC_SCALE_SHOW_ALL, G_PARAM_READWRITE));

  /**
   * SwfdecPlayer::invalidate:
   * @player: the #SwfdecPlayer affected
   * @x: x coordinate of invalid region
   * @y: y coordinate of invalid region
   * @width: width of invalid region
   * @height: height of invalid region
   *
   * This signal is emitted whenever graphical elements inside the player have 
   * changed. The coordinates describe the smallest rectangle that includes all
   * changes.
   */
  signals[INVALIDATE] = g_signal_new ("invalidate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, swfdec_marshal_VOID__DOUBLE_DOUBLE_DOUBLE_DOUBLE,
      G_TYPE_NONE, 4, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  /**
   * SwfdecPlayer::advance:
   * @player: the #SwfdecPlayer affected
   * @msecs: the amount of milliseconds the player will advance
   * @audio_samples: number of frames the audio is advanced (in 44100Hz steps)
   *
   * Emitted whenever the player advances.
   */
  signals[ADVANCE] = g_signal_new ("advance", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SwfdecPlayerClass, advance), 
      NULL, NULL, swfdec_marshal_VOID__UINT_UINT,
      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  /**
   * SwfdecPlayer::handle-mouse:
   * @player: the #SwfdecPlayer affected
   * @x: new x coordinate of the mouse
   * @y: new y coordinate of the mouse
   * @button: 1 if the button is pressed, 0 if not
   *
   * this signal is emitted whenever @player should respond to a mouse event. If
   * any of the handlers returns TRUE, swfdec_player_handle_mouse() will return 
   * TRUE. Note that unlike many event handlers in gtk, returning TRUE will not 
   * stop further event handlers from being invoked. Use g_signal_stop_emission()
   * in that case.
   *
   * Returns: TRUE if this handler handles the event. 
   **/
  signals[HANDLE_MOUSE] = g_signal_new ("handle-mouse", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SwfdecPlayerClass, handle_mouse), 
      swfdec_accumulate_or, NULL, swfdec_marshal_BOOLEAN__DOUBLE_DOUBLE_INT,
      G_TYPE_BOOLEAN, 3, G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INT);
  /**
   * SwfdecPlayer::audio-added:
   * @player: the #SwfdecPlayer affected
   * @audio: the audio stream that was added
   *
   * Emitted whenever a new audio stream was added to @player.
   */
  signals[AUDIO_ADDED] = g_signal_new ("audio-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, SWFDEC_TYPE_AUDIO);
  /**
   * SwfdecPlayer::audio-removed:
   * @player: the #SwfdecPlayer affected
   * @audio: the audio stream that was removed
   *
   * Emitted whenever an audio stream was removed from @player. The stream will 
   * have been added with the SwfdecPlayer::audio-added signal previously. 
   */
  signals[AUDIO_REMOVED] = g_signal_new ("audio-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, SWFDEC_TYPE_AUDIO);
  /**
   * SwfdecPlayer::fscommand:
   * @player: the #SwfdecPlayer affected
   * @command: the command to execute
   * @paramter: parameter to pass to the command. The parameter depends on the 
   *            function.
   *
   * This signal is emited whenever a Flash script command (also known as 
   * fscommand) is encountered. This method is ued by the Flash file to
   * communicate with the hosting environment. In web browsers it is used to 
   * call Javascript functions. Standalone Flash players understand a limited 
   * set of functions. They vary from player to player, but the most common are 
   * listed here: <itemizedlist>
   * <listitem><para>"quit": quits the player.</para></listitem>
   * <listitem><para>"fullscreen": A boolean setting (parameter is "true" or 
   * "false") that sets the player into fullscreen mode.</para></listitem>
   * <listitem><para>"allowscale": A boolean setting that tells the player to
   * not scale the Flash application.</para></listitem>
   * <listitem><para>"showmenu": A boolean setting that tells the Flash player
   * to not show its own entries in the right-click menu.</para></listitem>
   * <listitem><para>"exec": Run an external executable. The parameter 
   * specifies the path.</para></listitem>
   * <listitem><para>"trapallkeys": A boolean setting that tells the Flash 
   * player to pass all key events to the Flash application instead of using it
   * for keyboard shortcuts or similar.</para></listitem>
   * </itemizedlist>
   */
  signals[FSCOMMAND] = g_signal_new ("fscommand", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, swfdec_marshal_VOID__STRING_STRING,
      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
  /**
   * SwfdecPlayer::launch:
   * @player: the #SwfdecPlayer affected
   * @url: URL to open
   * @target: target to load the URL into
   *
   * Emitted whenever the @player encounters an URL that should be loaded into 
   * a target the Flash player does not recognize. In most cases this happens 
   * when the user clicks a link in an embedded Flash movie that should open a
   * new web page.
   * The effect of calling any swfdec functions on the emitting @player is undefined.
   */
  signals[LAUNCH] = g_signal_new ("launch", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, swfdec_marshal_VOID__STRING_STRING,
      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

  context_class->mark = swfdec_player_mark;
  context_class->get_time = swfdec_player_get_time;

  klass->advance = swfdec_player_do_advance;
  klass->handle_mouse = swfdec_player_do_handle_mouse;
}

static void
swfdec_player_init (SwfdecPlayer *player)
{
  player->registered_classes = g_hash_table_new (g_direct_hash, g_direct_equal);

  player->actions = swfdec_ring_buffer_new_for_type (SwfdecPlayerAction, 16);
  player->external_actions = swfdec_ring_buffer_new_for_type (SwfdecPlayerAction, 8);
  player->cache = swfdec_cache_new (50 * 1024 * 1024); /* 100 MB */
  player->bgcolor = SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0xFF);

  player->mouse_visible = TRUE;
  player->mouse_cursor = SWFDEC_MOUSE_CURSOR_NORMAL;
  player->iterate_timeout.callback = swfdec_player_iterate;
  player->init_queue = g_queue_new ();
  player->construct_queue = g_queue_new ();
  player->stage_width = -1;
  player->stage_height = -1;
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

SwfdecMovie *
swfdec_player_add_level_from_loader (SwfdecPlayer *player, guint depth,
    SwfdecLoader *loader, const char *variables)
{
  SwfdecMovie *movie;
  const char *name;

  swfdec_player_remove_level (player, depth);
  name = swfdec_as_context_give_string (SWFDEC_AS_CONTEXT (player), g_strdup_printf ("_level%u", depth));
  movie = swfdec_movie_new (player, depth - 16384, NULL, NULL, name);
  movie->name = SWFDEC_AS_STR_EMPTY;
  swfdec_swf_instance_new (SWFDEC_SPRITE_MOVIE (movie), loader);
  g_object_unref (loader);
  if (variables)
    swfdec_movie_set_variables (movie, variables);
  return movie;
}

void
swfdec_player_remove_level (SwfdecPlayer *player, guint depth)
{
  GList *walk;
  int real_depth;

  real_depth = (int) depth - 16384;

  for (walk = player->roots; walk; walk = walk->next) {
    SwfdecMovie *movie = walk->data;

    if (movie->depth < real_depth)
      continue;

    if (movie->depth == real_depth) {
      SWFDEC_DEBUG ("remove existing movie _level%u", depth);
      swfdec_movie_remove (movie);
      return;
    }
    break;
  }
  SWFDEC_LOG ("no movie to remove at level %u", depth);
}

SwfdecLoader *
swfdec_player_load (SwfdecPlayer *player, const char *url)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  g_assert (player->loader);
  return swfdec_loader_load (player->loader, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL, 0);
}

void
swfdec_player_launch (SwfdecPlayer *player, const char *url, const char *target)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (url != NULL);
  g_return_if_fail (target != NULL);

  if (g_str_has_prefix (url, "FSCommand:")) {
    const char *command = url + strlen ("FSCommand:");
    g_signal_emit (player, signals[FSCOMMAND], 0, command, target);
    return;
  }
  g_signal_emit (player, signals[LAUNCH], 0, url, target);
}

/**
 * swfdec_player_initialize:
 * @player: a #SwfdecPlayer
 * @version: Flash version to use
 * @rate: framerate in 256th or 0 for undefined
 * @width: width of movie
 * @height: height of movie
 *
 * Initializes the player to the given @version, @width, @height and @rate. If 
 * the player is already initialized, this function does nothing.
 **/
void
swfdec_player_initialize (SwfdecPlayer *player, guint version, 
    guint rate, guint width, guint height)
{
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  if (swfdec_player_is_initialized (player))
    return;
  
  context = SWFDEC_AS_CONTEXT (player);
  swfdec_as_context_startup (context, version);
  /* reset state for initialization */
  /* FIXME: have a better way to do this */
  if (context->state == SWFDEC_AS_CONTEXT_RUNNING) {
    context->state = SWFDEC_AS_CONTEXT_NEW;
    swfdec_player_init_global (player, version);
    swfdec_sprite_movie_init_context (player, version);
    swfdec_video_movie_init_context (player, version);
    swfdec_movie_color_init_context (player, version);
    swfdec_net_connection_init_context (player, version);
    swfdec_net_stream_init_context (player, version);
    swfdec_xml_init_context (player, version);
    if (version > 4) {
      SwfdecBits bits;
      SwfdecScript *script;
      swfdec_bits_init_data (&bits, swfdec_initialize, sizeof (swfdec_initialize));
      script = swfdec_script_new (&bits, "init", version);
      g_assert (script);
      swfdec_as_object_run (context->global, script);
      swfdec_script_unref (script);
    }
    if (context->state == SWFDEC_AS_CONTEXT_NEW) {
      context->state = SWFDEC_AS_CONTEXT_RUNNING;
      swfdec_as_object_set_constructor (player->roots->data, player->MovieClip, FALSE);
    }
  }
  SWFDEC_INFO ("initializing player to size %ux%u", width, height);
  player->rate = rate;
  player->width = width;
  player->height = height;
  player->initialized = TRUE;
  if (rate) {
    player->iterate_timeout.timestamp = player->time + SWFDEC_TICKS_PER_SECOND * 256 / rate;
    swfdec_player_add_timeout (player, &player->iterate_timeout);
    SWFDEC_LOG ("initialized iterate timeout %p to %"G_GUINT64_FORMAT" (now %"G_GUINT64_FORMAT")",
	&player->iterate_timeout, player->iterate_timeout.timestamp, player->time);
  }
  g_object_notify (G_OBJECT (player), "initialized");
  swfdec_player_update_scale (player);
}

/**
 * swfdec_player_get_export_class:
 * @player: a #SwfdecPlayer
 * @name: garbage-collected string naming the export
 *
 * Looks up the constructor for characters that are exported using @name.
 *
 * Returns: a #SwfdecAsObject naming the constructor or %NULL if none
 **/
SwfdecAsObject *
swfdec_player_get_export_class (SwfdecPlayer *player, const char *name)
{
  SwfdecAsObject *ret;
  
  ret = g_hash_table_lookup (player->registered_classes, name);
  if (ret) {
    SWFDEC_LOG ("found registered class %p for %s\n", ret, name);
    return ret;
  }
  return player->MovieClip;
}

/**
 * swfdec_player_set_export_class:
 * @player: a #SwfdecPlayer
 * @name: garbage-collected string naming the export
 * @object: object to use as constructor or %NULL for none
 *
 * Sets the constructor to be used for instances created using the object 
 * exported with @name.
 **/
void
swfdec_player_set_export_class (SwfdecPlayer *player, const char *name, SwfdecAsObject *object)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (name != NULL);
  g_return_if_fail (object == NULL || SWFDEC_IS_AS_OBJECT (object));

  if (object) {
    SWFDEC_LOG ("setting class %p for %s\n", object, name);
    g_hash_table_insert (player->registered_classes, (gpointer) name, object);
  } else {
    g_hash_table_remove (player->registered_classes, name);
  }
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
 * For details, see swfdec_player_set_loader_with_variables().
 **/
void
swfdec_player_set_loader (SwfdecPlayer *player, SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (player->roots == NULL);
  g_return_if_fail (SWFDEC_IS_LOADER (loader));

  swfdec_player_set_loader_with_variables (player, loader, NULL);
}

/**
 * swfdec_player_set_loader_with_variables:
 * @player: a #SwfdecPlayer
 * @loader: the loader to use for this player. Takes ownership of the given loader.
 * @variables: a string that is checked to be in 'application/x-www-form-urlencoded'
 *             syntax describing the arguments to set on the new player or NULL for
 *             none.
 *
 * Sets the loader for the main data. This function only works if no loader has 
 * been set on @player yet.
 * If the @variables are set and validate, they will be set as properties on the 
 * root movie. 
 * <note>If you want to capture events during the setup process, you want to 
 * connect your signal handlers before calling swfdec_player_set_loader() and
 * not use conveniencse functions such as swfdec_player_new_from_file().</note>
 **/
void
swfdec_player_set_loader_with_variables (SwfdecPlayer *player, SwfdecLoader *loader,
    const char *variables)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (player->roots == NULL);
  g_return_if_fail (SWFDEC_IS_LOADER (loader));

  player->loader = loader;
  g_object_ref (loader);
  swfdec_player_add_level_from_loader (player, 0, loader, variables);
  swfdec_loader_parse (loader);
}

/**
 * swfdec_player_new_from_file:
 * @filename: name of the file to play
 *
 * Creates a player to play back the given file. If the file does not
 * exist or another error occurs, the player will be in an error state and not
 * be initialized.
 * This function calls swfdec_init () for you if it wasn't called before.
 *
 * Returns: a new player
 **/
SwfdecPlayer *
swfdec_player_new_from_file (const char *filename)
{
  SwfdecLoader *loader;
  SwfdecPlayer *player;

  g_return_val_if_fail (filename != NULL, NULL);

  loader = swfdec_loader_new_from_file (filename);
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
}

/**
 * swfdec_player_handle_mouse:
 * @player: a #SwfdecPlayer
 * @x: x coordinate of mouse
 * @y: y coordinate of mouse
 * @button: 1 for pressed, 0 for not pressed
 *
 * Updates the current mouse status. If the mouse has left the area of @player,
 * you should pass values outside the movie size for @x and @y. You will 
 * probably want to call swfdec_player_advance() before to update the player to
 * the correct time when calling this function.
 *
 * Returns: TRUE if the mouse event was handled. A mouse event may not be 
 *          handled if the user clicked on a translucent area for example.
 **/
gboolean
swfdec_player_handle_mouse (SwfdecPlayer *player, 
    double x, double y, int button)
{
  gboolean ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (button == 0 || button == 1, FALSE);

  g_signal_emit (player, signals[HANDLE_MOUSE], 0, x, y, button, &ret);

  return ret;
}

/**
 * swfdec_player_render:
 * @player: a #SwfdecPlayer
 * @cr: #cairo_t to render to
 * @x: x coordinate of top left position to render
 * @y: y coordinate of top left position to render
 * @width: width of area to render or 0 for full width
 * @height: height of area to render or 0 for full height
 *
 * Renders the given area of the current frame to @cr.
 **/
void
swfdec_player_render (SwfdecPlayer *player, cairo_t *cr, 
    double x, double y, double width, double height)
{
  static const SwfdecColorTransform trans = { 256, 0, 256, 0, 256, 0, 256, 0 };
  GList *walk;
  SwfdecRect real;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (width >= 0.0);
  g_return_if_fail (height >= 0.0);

  /* FIXME: fail when !initialized? */
  if (!swfdec_player_is_initialized (player))
    return;

  if (width == 0.0)
    width = player->stage_width;
  if (height == 0.0)
    height = player->stage_height;
  /* clip the area */
  cairo_save (cr);
  cairo_rectangle (cr, x, y, width, height);
  cairo_clip (cr);
  /* compute the rectangle */
  x -= player->offset_x;
  y -= player->offset_y;
  real.x0 = floor (x * SWFDEC_TWIPS_SCALE_FACTOR) / player->scale_x;
  real.y0 = floor (y * SWFDEC_TWIPS_SCALE_FACTOR) / player->scale_y;
  real.x1 = ceil ((x + width) * SWFDEC_TWIPS_SCALE_FACTOR) / player->scale_x;
  real.y1 = ceil ((y + height) * SWFDEC_TWIPS_SCALE_FACTOR) / player->scale_y;
  SWFDEC_INFO ("=== %p: START RENDER, area %g %g  %g %g ===", player, 
      real.x0, real.y0, real.x1, real.y1);
  /* convert the cairo matrix */
  cairo_translate (cr, player->offset_x, player->offset_y);
  cairo_scale (cr, player->scale_x / SWFDEC_TWIPS_SCALE_FACTOR, player->scale_y / SWFDEC_TWIPS_SCALE_FACTOR);
  swfdec_color_set_source (cr, player->bgcolor);
  cairo_paint (cr);

  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_render (walk->data, cr, &trans, &real, TRUE);
  }
  SWFDEC_INFO ("=== %p: END RENDER ===", player);
  cairo_restore (cr);
}

/**
 * swfdec_player_advance:
 * @player: the #SwfdecPlayer to advance
 * @msecs: number of milliseconds to advance
 *
 * Advances @player by @msecs. You should make sure to call this function as
 * often as the SwfdecPlayer::next-event property indicates.
 **/
void
swfdec_player_advance (SwfdecPlayer *player, guint msecs)
{
  guint frames;
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (msecs > 0);

  frames = SWFDEC_TICKS_TO_SAMPLES (player->time + SWFDEC_MSECS_TO_TICKS (msecs))
    - SWFDEC_TICKS_TO_SAMPLES (player->time);
  g_signal_emit (player, signals[ADVANCE], 0, msecs, frames);
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

  return player->initialized;
}

/**
 * swfdec_player_get_next_event:
 * @player: ia #SwfdecPlayer
 *
 * Queries how long to the next event. This is the next time when you should 
 * call swfdec_player_advance() to forward to.
 *
 * Returns: number of milliseconds until next event or 0 if no outstanding event
 **/
guint
swfdec_player_get_next_event (SwfdecPlayer *player)
{
  SwfdecTick tick;
  guint ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);

  tick = swfdec_player_get_next_event_time (player);
  ret = SWFDEC_TICKS_TO_MSECS (tick);
  if (tick % (SWFDEC_TICKS_PER_SECOND / 1000))
    ret++;

  return ret;
}

/**
 * swfdec_player_get_rate:
 * @player: a #SwfdecPlayer
 *
 * Queries the framerate of this movie. This number specifies the number
 * of frames that are supposed to pass per second. It is a 
 * multiple of 1/256. It is possible that the movie has no framerate if it does
 * not display a Flash movie but an FLV video for example. This does not mean
 * it will not change however.
 *
 * Returns: The framerate of this movie or 0 if it isn't known yet or the
 *          movie doesn't have a framerate.
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
 * @width: integer to store the width in or %NULL
 * @height: integer to store the height in or %NULL
 *
 * If the default size of the movie is initialized, fills in @width and @height 
 * with the size. Otherwise @width and @height are set to 0.
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

/**
 * swfdec_player_get_size:
 * @player: a #SwfdecPlayer
 * @width: integer to store the width in or %NULL
 * @height: integer to store the height in or %NULL
 *
 * Gets the currently set image size. If the default width or height should be 
 * used, the width or height respectively is set to -1.
 **/
void
swfdec_player_get_size (SwfdecPlayer *player, int *width, int *height)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  if (width)
    *width = player->stage_width;
  if (height)
    *height = player->stage_height;
}

static void
swfdec_player_set_size_internal (SwfdecPlayer *player, int width, int height)
{
  gboolean changed = FALSE;

  if (player->stage_width != width) {
    player->stage_width = width;
    g_object_notify (G_OBJECT (player), "width");
    changed = TRUE;
  }
  if (player->stage_height != height) {
    player->stage_height = height;
    g_object_notify (G_OBJECT (player), "height");
    changed = TRUE;
  }
  swfdec_player_update_scale (player);
  if (changed && player->scale_mode == SWFDEC_SCALE_NONE) {
    swfdec_player_broadcast (player, SWFDEC_AS_STR_Stage, SWFDEC_AS_STR_onResize);
  }
}

/**
 * swfdec_player_set_size:
 * @player: a #SwfdecPlayer
 * @width: desired width of the movie or -1 for default
 * @height: desired height of the movie or -1 for default
 *
 * Sets the image size to the given values. The image size is what the area that
 * the @player will render and advocate with scripts.
 * <note>Calling this function or setting the corresponding properties causes 
 * the emission of an event inside the script engine for listeners registered on
 * the Stage object.</note>
 **/
void
swfdec_player_set_size (SwfdecPlayer *player, int width, int height)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  swfdec_player_lock (player);
  swfdec_player_set_size_internal (player, width, height);
  swfdec_player_perform_actions (player);
  swfdec_player_unlock (player);
}

/**
 * swfdec_player_get_audio:
 * @player: a #SwfdecPlayer
 *
 * Returns a list of all currently active audio streams in @player.
 *
 * Returns: A #GList of #SwfdecAudio. You must not modify or free this list.
 **/
/* FIXME: I don't like this function */
const GList *
swfdec_player_get_audio (SwfdecPlayer *	player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  return player->audio;
}

/**
 * swfdec_player_get_background_color:
 * @player: a #SwfdecPlayer
 *
 * Gets the current background color. The color will be an ARGB-quad, with the 
 * MSB being the alpha value.
 *
 * Returns: the background color as an ARGB value
 **/
guint
swfdec_player_get_background_color (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0xFF));

  return player->bgcolor;
}

/**
 * swfdec_player_set_background_color:
 * @player: a #SwfdecPlayer
 * @color: new color to use as background color
 *
 * Sets a new background color as an ARGB value. To get transparency, set the 
 * value to 0. To get a black beackground, use 0xFF000000.
 **/
void
swfdec_player_set_background_color (SwfdecPlayer *player, guint color)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  player->bgcolor_set = TRUE;
  if (player->bgcolor == color)
    return;
  g_object_notify (G_OBJECT (player), "background-color");
  if (swfdec_player_is_initialized (player)) {
    g_signal_emit (player, signals[INVALIDATE], 0, 0.0, 0.0, 
	(double) player->width, (double) player->height);
  }
}

/**
 * swfdec_player_get_scale_mode:
 * @player: a #SwfdecPlayer
 *
 * Gets the currrent mode used for scaling the movie. See #SwfdecScaleMode for 
 * the different modes.
 *
 * Returns: the current scale mode
 **/
SwfdecScaleMode
swfdec_player_get_scale_mode (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), SWFDEC_SCALE_SHOW_ALL);

  return player->scale_mode;
}

/**
 * swfdec_player_set_scale_mode:
 * @player: a #SwfdecPlayer
 * @mode: a #SwfdecScaleMode
 *
 * Sets the currrent mode used for scaling the movie. See #SwfdecScaleMode for 
 * the different modes.
 **/
void
swfdec_player_set_scale_mode (SwfdecPlayer *player, SwfdecScaleMode mode)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  if (player->scale_mode != mode) {
    player->scale_mode = mode;
    swfdec_player_update_scale (player);
    g_object_notify (G_OBJECT (player), "scale-mode");
  }
}

/**
 * swfdec_player_get_alignment:
 * @player: a #SwfdecPlayer
 *
 * Gets the alignment of the player. The alignment describes what point is used
 * as the anchor for drawing the contents. See #SwfdecAlignment for possible 
 * values.
 *
 * Returns: the current alignment
 **/
SwfdecAlignment
swfdec_player_get_alignment (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), SWFDEC_ALIGNMENT_CENTER);

  return swfdec_player_alignment_from_flags (player->align_flags);
}

/**
 * swfdec_player_set_alignment:
 * @player: a #SwfdecPlayer
 * @align: #SwfdecAlignment to set
 *
 * Sets the alignment to @align. For details about alignment, see 
 * swfdec_player_get_alignment() and #SwfdecAlignment.
 **/
void
swfdec_player_set_alignment (SwfdecPlayer *player, SwfdecAlignment align)
{
  guint flags;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  flags = swfdec_player_alignment_to_flags (align);
  swfdec_player_set_align_flags (player, flags);
}

void
swfdec_player_set_align_flags (SwfdecPlayer *player, guint flags)
{
  if (flags != player->align_flags) {
    player->align_flags = flags;
    swfdec_player_update_scale (player);
    g_object_notify (G_OBJECT (player), "alignment");
  }
}

