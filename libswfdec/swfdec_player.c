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

#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <liboil/liboil.h>

#include "swfdec_player_internal.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_internal.h"
#include "swfdec_button_movie.h" /* for mouse cursor */
#include "swfdec_cache.h"
#include "swfdec_debug.h"
#include "swfdec_enums.h"
#include "swfdec_event.h"
#include "swfdec_flash_security.h"
#include "swfdec_initialize.h"
#include "swfdec_internal.h"
#include "swfdec_loader_internal.h"
#include "swfdec_marshal.h"
#include "swfdec_movie.h"
#include "swfdec_script_internal.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_resource.h"
#include "swfdec_utils.h"

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

/**
 * SwfdecKey:
 * @SWFDEC_KEY_BACKSPACE: the backspace key
 * @SWFDEC_KEY_TAB: the tab key
 * @SWFDEC_KEY_CLEAR: the clear key
 * @SWFDEC_KEY_ENTER: the enter key
 * @SWFDEC_KEY_SHIFT: the shift key
 * @SWFDEC_KEY_CONTROL: the control key
 * @SWFDEC_KEY_ALT: the alt key
 * @SWFDEC_KEY_CAPS_LOCK: the caps lock key
 * @SWFDEC_KEY_ESCAPE: the escape key
 * @SWFDEC_KEY_SPACE: the space key
 * @SWFDEC_KEY_PAGE_UP: the page up key
 * @SWFDEC_KEY_PAGE_DOWN: the page down key
 * @SWFDEC_KEY_END: the end key
 * @SWFDEC_KEY_HOME: the home key
 * @SWFDEC_KEY_LEFT: the left key
 * @SWFDEC_KEY_UP: the up key
 * @SWFDEC_KEY_RIGHT: the right key
 * @SWFDEC_KEY_DOWN: the down key
 * @SWFDEC_KEY_INSERT: the insert key
 * @SWFDEC_KEY_DELETE: the delete key
 * @SWFDEC_KEY_HELP: the help key
 * @SWFDEC_KEY_0: the 0 key
 * @SWFDEC_KEY_1: the 1 key
 * @SWFDEC_KEY_2: the 2 key
 * @SWFDEC_KEY_3: the 3 key
 * @SWFDEC_KEY_4: the 4 key
 * @SWFDEC_KEY_5: the 5 key
 * @SWFDEC_KEY_6: the 6 key
 * @SWFDEC_KEY_7: the 7 key
 * @SWFDEC_KEY_8: the 8 key
 * @SWFDEC_KEY_9: the 9 key
 * @SWFDEC_KEY_A: the ! key
 * @SWFDEC_KEY_B: the B key
 * @SWFDEC_KEY_C: the C key
 * @SWFDEC_KEY_D: the D key
 * @SWFDEC_KEY_E: the E key
 * @SWFDEC_KEY_F: the F key
 * @SWFDEC_KEY_G: the G key
 * @SWFDEC_KEY_H: the H key
 * @SWFDEC_KEY_I: the I key
 * @SWFDEC_KEY_J: the J key
 * @SWFDEC_KEY_K: the K key
 * @SWFDEC_KEY_L: the L key
 * @SWFDEC_KEY_M: the M key
 * @SWFDEC_KEY_N: the N key
 * @SWFDEC_KEY_O: the O key
 * @SWFDEC_KEY_P: the P key
 * @SWFDEC_KEY_Q: the Q key
 * @SWFDEC_KEY_R: the R key
 * @SWFDEC_KEY_S: the S key
 * @SWFDEC_KEY_T: the T key
 * @SWFDEC_KEY_U: the U key
 * @SWFDEC_KEY_V: the V key
 * @SWFDEC_KEY_W: the W key
 * @SWFDEC_KEY_X: the X key
 * @SWFDEC_KEY_Y: the Y key
 * @SWFDEC_KEY_Z: the Z key
 * @SWFDEC_KEY_NUMPAD_0: the 0 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_1: the 1 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_2: the 2 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_3: the 3 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_4: the 4 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_5: the 5 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_6: the 6 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_7: the 7 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_8: the 8 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_9: the 9 key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_MULTIPLY: the multiply key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_ADD: the add key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_SUBTRACT: the subtract key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_DECIMAL: the decimal key on the numeric keypad
 * @SWFDEC_KEY_NUMPAD_DIVIDE: the divide key on the numeric keypad
 * @SWFDEC_KEY_F1: the F1 key
 * @SWFDEC_KEY_F2: the F2 key
 * @SWFDEC_KEY_F3: the F3 key
 * @SWFDEC_KEY_F4: the F4 key
 * @SWFDEC_KEY_F5: the F5 key
 * @SWFDEC_KEY_F6: the F6 key
 * @SWFDEC_KEY_F7: the F7 key
 * @SWFDEC_KEY_F8: the F8 key
 * @SWFDEC_KEY_F9: the F9 key
 * @SWFDEC_KEY_F10: the F10 key
 * @SWFDEC_KEY_F11: the F11 key
 * @SWFDEC_KEY_F12: the F12 key
 * @SWFDEC_KEY_F13: the F13 key
 * @SWFDEC_KEY_F14: the F14 key
 * @SWFDEC_KEY_F15: the F15 key
 * @SWFDEC_KEY_NUM_LOCK: the num lock key
 * @SWFDEC_KEY_SEMICOLON: the semicolon key (on English keyboards)
 * @SWFDEC_KEY_EQUAL: the equal key (on English keyboards)
 * @SWFDEC_KEY_MINUS: the minus key (on English keyboards)
 * @SWFDEC_KEY_SLASH: the slash key (on English keyboards)
 * @SWFDEC_KEY_GRAVE: the grave key (on English keyboards)
 * @SWFDEC_KEY_LEFT_BRACKET: the left bracket key (on English keyboards)
 * @SWFDEC_KEY_BACKSLASH: the backslash key (on English keyboards)
 * @SWFDEC_KEY_RIGHT_BRACKET: the right bracket key (on English keyboards)
 * @SWFDEC_KEY_APOSTROPHE: the apostrophe key (on English keyboards)
 *
 * Lists all known key codes in Swfdec and their meanings on an English 
 * keyboard.
 */

/*** Timeouts ***/

static SwfdecTick
swfdec_player_get_next_event_time (SwfdecPlayer *player)
{
  if (player->timeouts) {
    return ((SwfdecTimeout *) player->timeouts->data)->timestamp - player->time;
  } else {
    return G_MAXUINT64;
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
  g_return_if_fail (timeout->timestamp >= player->time);
  g_return_if_fail (timeout->callback != NULL);

  SWFDEC_LOG ("adding timeout %p in %"G_GUINT64_FORMAT" msecs", timeout, 
      SWFDEC_TICKS_TO_MSECS (timeout->timestamp - player->time));
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
  g_return_if_fail (timeout->timestamp >= player->time);
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

  SWFDEC_LOG ("executing action %p %p %p", 
      action->object, action->func, action->data);
  action->func (action->object, action->data);

  return TRUE;
}

static void
swfdec_player_perform_external_actions (SwfdecPlayer *player)
{
  SwfdecPlayerAction *action;
  guint i;

  /* remove timeout if it exists - do this before executing stuff below */
  if (player->external_timeout.callback) {
    swfdec_player_remove_timeout (player, &player->external_timeout);
    player->external_timeout.callback = NULL;
  }

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

  swfdec_player_perform_actions (player);
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
    swfdec_ring_buffer_set_size (player->external_actions,
	swfdec_ring_buffer_get_size (player->external_actions) + 16);
    action = swfdec_ring_buffer_push (player->external_actions);
    g_assert (action);
  }
  action->object = object;
  action->func = action_func;
  action->data = action_data;
  if (!player->external_timeout.callback) {
    /* trigger execution immediately.
     * But if initialized, keep at least 100ms from when the last external 
     * timeout triggered. This is a crude method to get around infinite loops
     * when script actions executed by external actions trigger another external
     * action that would execute instantly.
     */
    if (player->initialized) {
      player->external_timeout.timestamp = MAX (player->time,
	  player->external_timeout.timestamp + SWFDEC_MSECS_TO_TICKS (100));
    } else {
      player->external_timeout.timestamp = player->time;
    }
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
  HANDLE_KEY,
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
  PROP_SCALE,
  PROP_SYSTEM
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
    case PROP_SYSTEM:
      g_value_set_object (value, player->system);
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

  player->stage.width = player->stage_width >= 0 ? player->stage_width : (int) player->width;
  player->stage.height = player->stage_height >= 0 ? player->stage_height : (int) player->height;
  if (player->stage.height == 0 || player->stage.width == 0) {
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
    scale_x = (double) player->stage.width / player->width;
    scale_y = (double) player->stage.height / player->height;
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
  width = player->stage.width - ceil (player->width * player->scale_x);
  height = player->stage.height - ceil (player->height * player->scale_y);
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
    case PROP_SYSTEM:
      g_object_unref (player->system);
      if (g_value_get_object (value)) {
	player->system = SWFDEC_SYSTEM (g_value_dup_object (value));
      } else {
	player->system = swfdec_system_new ();
      }
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

  /* we do this here so references to GC'd objects get freed */
  G_OBJECT_CLASS (swfdec_player_parent_class)->dispose (object);

  swfdec_player_remove_all_external_actions (player, player);
#ifndef G_DISABLE_ASSERT
  {
    SwfdecPlayerAction *action;
    while ((action = swfdec_ring_buffer_pop (player->external_actions)) != NULL) {
      g_assert (action->object == NULL); /* skip removed actions */
    }
    while ((action = swfdec_ring_buffer_pop (player->actions)) != NULL) {
      g_assert (action->object == NULL); /* skip removed actions */
    }
  }
#endif
  swfdec_ring_buffer_free (player->external_actions);
  swfdec_ring_buffer_free (player->actions);
  g_assert (player->movies == NULL);
  g_assert (player->audio == NULL);
  if (player->external_timeout.callback)
    swfdec_player_remove_timeout (player, &player->external_timeout);
  if (player->rate) {
    swfdec_player_remove_timeout (player, &player->iterate_timeout);
  }
  g_assert (player->timeouts == NULL);
  g_list_free (player->intervals);
  g_list_free (player->load_objects);
  player->intervals = NULL;
  g_assert (g_queue_is_empty (player->init_queue));
  g_assert (g_queue_is_empty (player->construct_queue));
  g_queue_free (player->init_queue);
  g_queue_free (player->construct_queue);
  swfdec_cache_unref (player->cache);
  if (player->resource) {
    g_object_unref (player->resource);
    player->resource = NULL;
  }
  if (player->system) {
    g_object_unref (player->system);
    player->system = NULL;
  }
  g_array_free (player->invalidations, TRUE);
  player->invalidations = NULL;
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
  swfdec_player_stage_to_global (player, &x, &y);
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
  SWFDEC_LOG ("mouse is at %g %g, originally (%g %g)", x, y, player->mouse_x, player->mouse_y);
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
    swfdec_player_stage_to_global (player, &player->mouse_drag_x, &player->mouse_drag_y);
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
    double x, y;
    /* if the mouse button is pressed the grab widget stays the same (I think) */
    x = player->mouse_x;
    y = player->mouse_y;
    swfdec_player_stage_to_global (player, &x, &y);
    for (walk = g_list_last (player->roots); walk; walk = walk->prev) {
      mouse_grab = swfdec_movie_get_movie_at (walk->data, x, y);
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
  if (!swfdec_rectangle_is_empty (&player->invalid_extents)) {
    g_signal_emit (player, signals[INVALIDATE], 0, &player->invalid_extents,
	player->invalidations->data, player->invalidations->len);
    swfdec_rectangle_init_empty (&player->invalid_extents);
    g_array_set_size (player->invalidations, 0);
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
swfdec_player_do_handle_key (SwfdecPlayer *player, guint keycode, guint character, gboolean down)
{
  g_assert (keycode < 256);

  swfdec_player_lock (player);
  /* set the correct variables */
  player->last_keycode = keycode;
  player->last_character = character;
  if (down) {
    player->key_pressed[keycode / 8] |= 1 << keycode % 8;
  } else {
    player->key_pressed[keycode / 8] &= ~(1 << keycode % 8);
  }
  swfdec_player_broadcast (player, SWFDEC_AS_STR_Key, down ? SWFDEC_AS_STR_onKeyDown : SWFDEC_AS_STR_onKeyUp);
  swfdec_player_perform_actions (player);
  swfdec_player_unlock (player);

  return TRUE;
}

static gboolean
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

  /* FIXME: allow events to pass through */
  return TRUE;
}

void
swfdec_player_global_to_stage (SwfdecPlayer *player, double *x, double *y)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  *x = *x / SWFDEC_TWIPS_SCALE_FACTOR * player->scale_x + player->offset_x;
  *y = *y / SWFDEC_TWIPS_SCALE_FACTOR * player->scale_y + player->offset_y;
}

void
swfdec_player_stage_to_global (SwfdecPlayer *player, double *x, double *y)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  *x = (*x - player->offset_x) / player->scale_x * SWFDEC_TWIPS_SCALE_FACTOR;
  *y = (*y - player->offset_y) / player->scale_y * SWFDEC_TWIPS_SCALE_FACTOR;
}

static void
swfdec_player_iterate (SwfdecTimeout *timeout)
{
  SwfdecPlayer *player = SWFDEC_PLAYER ((guint8 *) timeout - G_STRUCT_OFFSET (SwfdecPlayer, iterate_timeout));
  GList *walk;

  swfdec_player_perform_external_actions (player);
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
swfdec_player_advance_audio (SwfdecPlayer *player, guint samples)
{
  SwfdecAudio *audio;
  GList *walk;

  if (samples == 0)
    return;

  /* don't use for loop here, because we need to advance walk before 
   * removing the audio */
  walk = player->audio;
  while (walk) {
    audio = walk->data;
    walk = walk->next;
    if (swfdec_audio_iterate (audio, samples) == 0)
      swfdec_audio_remove (audio);
  }
}

static void
swfdec_player_do_advance (SwfdecPlayer *player, gulong msecs, guint audio_samples)
{
  SwfdecTimeout *timeout;
  SwfdecTick target_time;
  guint frames_now;
  
  swfdec_player_lock (player);
  target_time = player->time + SWFDEC_MSECS_TO_TICKS (msecs);
  SWFDEC_DEBUG ("advancing %lu msecs (%u audio frames)", msecs, audio_samples);

  for (timeout = player->timeouts ? player->timeouts->data : NULL;
       timeout && timeout->timestamp <= target_time; 
       timeout = player->timeouts ? player->timeouts->data : NULL) {
    player->timeouts = g_list_remove (player->timeouts, timeout);
    frames_now = SWFDEC_TICKS_TO_SAMPLES (timeout->timestamp) -
      SWFDEC_TICKS_TO_SAMPLES (player->time);
    player->time = timeout->timestamp;
    swfdec_player_advance_audio (player, frames_now);
    audio_samples -= frames_now;
    SWFDEC_LOG ("activating timeout %p now (timeout is %"G_GUINT64_FORMAT", target time is %"G_GUINT64_FORMAT,
	timeout, timeout->timestamp, target_time);
    timeout->callback (timeout);
    swfdec_player_perform_actions (player);
  }
  if (target_time > player->time) {
    frames_now = SWFDEC_TICKS_TO_SAMPLES (target_time) -
      SWFDEC_TICKS_TO_SAMPLES (player->time);
    player->time = target_time;
    swfdec_player_advance_audio (player, frames_now);
    audio_samples -= frames_now;
  }
  g_assert (audio_samples == 0);
  
  swfdec_player_unlock (player);
}

void
swfdec_player_perform_actions (SwfdecPlayer *player)
{
  GList *walk;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  while (swfdec_player_do_action (player));
  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_update (walk->data);
  }
  /* update the state of the mouse when stuff below it moved */
  if (swfdec_rectangle_contains_point (&player->invalid_extents, player->mouse_x, player->mouse_y)) {
    SWFDEC_INFO ("=== NEED TO UPDATE mouse post-iteration ===");
    swfdec_player_update_mouse_position (player);
    while (swfdec_player_do_action (player));
    for (walk = player->roots; walk; walk = walk->next) {
      swfdec_movie_update (walk->data);
    }
  }
}

/* used for breakpoints */
void
swfdec_player_lock_soft (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_assert (swfdec_rectangle_is_empty (&player->invalid_extents));

  g_object_freeze_notify (G_OBJECT (player));
  SWFDEC_DEBUG ("LOCKED");
}

void
swfdec_player_lock (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_assert (swfdec_ring_buffer_get_n_elements (player->actions) == 0);

  g_object_ref (player);
  swfdec_player_lock_soft (player);
}

/* used for breakpoints */
void
swfdec_player_unlock_soft (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  SWFDEC_DEBUG ("UNLOCK");
  swfdec_player_update_mouse_cursor (player);
  g_object_thaw_notify (G_OBJECT (player));
  swfdec_player_emit_signals (player);
}

void
swfdec_player_unlock (SwfdecPlayer *player)
{
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_assert (swfdec_ring_buffer_get_n_elements (player->actions) == 0);
  context = SWFDEC_AS_CONTEXT (player);
  g_return_if_fail (context->state != SWFDEC_AS_CONTEXT_INTERRUPTED);

  if (context->state == SWFDEC_AS_CONTEXT_RUNNING)
    swfdec_as_context_maybe_gc (SWFDEC_AS_CONTEXT (player));
  swfdec_player_unlock_soft (player);
  g_object_unref (player);
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

  g_hash_table_foreach (player->registered_classes, swfdec_player_mark_string_object, NULL);
  swfdec_as_object_mark (player->MovieClip);
  swfdec_as_object_mark (player->Video);
  g_list_foreach (player->roots, (GFunc) swfdec_as_object_mark, NULL);
  g_list_foreach (player->intervals, (GFunc) swfdec_as_object_mark, NULL);
  g_list_foreach (player->load_objects, (GFunc) swfdec_as_object_mark, NULL);

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
      g_param_spec_long ("next-event", "next event", "how many milliseconds until the next event or 0 when no event pending",
	  -1, G_MAXLONG, -1, G_PARAM_READABLE));
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
  g_object_class_install_property (object_class, PROP_SCALE,
      g_param_spec_object ("system", "system", "object holding system information",
	  SWFDEC_TYPE_SYSTEM, G_PARAM_READWRITE));

  /**
   * SwfdecPlayer::invalidate:
   * @player: the #SwfdecPlayer affected
   * @extents: the smallest rectangle enclosing the full region of changes
   * @rectangles: a number of smaller rectangles for fine-grained control over 
   *              changes
   * @n_rectangles: number of rectangles in @rectangles
   *
   * This signal is emitted whenever graphical elements inside the player have 
   * changed. It provides two ways to look at the changes: By looking at the
   * @extents parameter, it provides a simple way to get a single rectangle that
   * encloses all changes. By looking at the @rectangles array, you can get
   * finer control over changes which is very useful if your rendering system 
   * provides a way to handle regions.
   */
  signals[INVALIDATE] = g_signal_new ("invalidate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, swfdec_marshal_VOID__BOXED_POINTER_UINT,
      G_TYPE_NONE, 3, SWFDEC_TYPE_RECTANGLE, G_TYPE_POINTER, G_TYPE_UINT);
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
      NULL, NULL, swfdec_marshal_VOID__ULONG_UINT,
      G_TYPE_NONE, 2, G_TYPE_ULONG, G_TYPE_UINT);
  /**
   * SwfdecPlayer::handle-key:
   * @player: the #SwfdecPlayer affected
   * @key: #SwfdecKey that was pressed or released
   * @pressed: %TRUE if the @key was pressed or %FALSE if it was released
   *
   * This signal is emitted whenever @player should respond to a key event. If
   * any of the handlers returns TRUE, swfdec_player_key_press() or 
   * swfdec_player_key_release() will return TRUE. Note that unlike many event 
   * handlers in gtk, returning TRUE will not stop further event handlers from 
   * being invoked. Use g_signal_stop_emission() in that case.
   *
   * Returns: TRUE if this handler handles the event. 
   **/
  signals[HANDLE_KEY] = g_signal_new ("handle-key", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (SwfdecPlayerClass, handle_key), 
      swfdec_accumulate_or, NULL, swfdec_marshal_BOOLEAN__UINT_UINT_BOOLEAN,
      G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_BOOLEAN);
  /**
   * SwfdecPlayer::handle-mouse:
   * @player: the #SwfdecPlayer affected
   * @x: new x coordinate of the mouse
   * @y: new y coordinate of the mouse
   * @button: 1 if the button is pressed, 0 if not
   *
   * This signal is emitted whenever @player should respond to a mouse event. If
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
   * @command: the command to execute. This is a lower case string.
   * @parameter: parameter to pass to the command. The parameter depends on the 
   *             function.
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
   * @request: the type of request
   * @url: URL to open
   * @target: target to load the URL into
   * @data: optional data to pass on with the request. Will be of mime type
   *        application/x-www-form-urlencoded. Can be %NULL indicating no data
   *        should be passed.
   *
   * Emitted whenever the @player encounters an URL that should be loaded into 
   * a target the Flash player does not recognize. In most cases this happens 
   * when the user clicks a link in an embedded Flash movie that should open a
   * new web page.
   * The effect of calling any swfdec functions on the emitting @player is undefined.
   */
  signals[LAUNCH] = g_signal_new ("launch", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, swfdec_marshal_VOID__ENUM_STRING_STRING_BOXED,
      G_TYPE_NONE, 4, SWFDEC_TYPE_LOADER_REQUEST, G_TYPE_STRING, G_TYPE_STRING, 
      SWFDEC_TYPE_BUFFER);

  context_class->mark = swfdec_player_mark;
  context_class->get_time = swfdec_player_get_time;

  klass->advance = swfdec_player_do_advance;
  klass->handle_key = swfdec_player_do_handle_key;
  klass->handle_mouse = swfdec_player_do_handle_mouse;
}

static void
swfdec_player_init (SwfdecPlayer *player)
{
  player->system = swfdec_system_new ();
  player->registered_classes = g_hash_table_new (g_direct_hash, g_direct_equal);

  player->actions = swfdec_ring_buffer_new_for_type (SwfdecPlayerAction, 16);
  player->external_actions = swfdec_ring_buffer_new_for_type (SwfdecPlayerAction, 8);
  player->cache = swfdec_cache_new (50 * 1024 * 1024); /* 100 MB */
  player->bgcolor = SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0xFF);

  player->invalidations = g_array_new (FALSE, FALSE, sizeof (SwfdecRectangle));
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
swfdec_player_stop_sounds (SwfdecPlayer *player, SwfdecAudioRemoveFunc func, gpointer data)
{
  GList *walk;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (func);

  walk = player->audio;
  while (walk) {
    SwfdecAudio *audio = walk->data;
    walk = walk->next;
    if (func (audio, data))
      swfdec_audio_remove (audio);
  }
}

/* rect is in global coordinates */
void
swfdec_player_invalidate (SwfdecPlayer *player, const SwfdecRect *rect)
{
  SwfdecRectangle r;
  SwfdecRect tmp;
  guint i;

  if (swfdec_rect_is_empty (rect)) {
    g_assert_not_reached ();
    return;
  }

  tmp = *rect;
  swfdec_player_global_to_stage (player, &tmp.x0, &tmp.y0);
  swfdec_player_global_to_stage (player, &tmp.x1, &tmp.y1);
  swfdec_rectangle_init_rect (&r, &tmp);
  /* FIXME: currently we clamp the rectangle to the visible area, it might
   * be useful to allow out-of-bounds drawing. In that case this needs to be
   * changed */
  swfdec_rectangle_intersect (&r, &r, &player->stage);
  if (swfdec_rectangle_is_empty (&r))
    return;

  /* FIXME: get region code into swfdec? */
  for (i = 0; i < player->invalidations->len; i++) {
    SwfdecRectangle *cur = &g_array_index (player->invalidations, SwfdecRectangle, i);
    if (swfdec_rectangle_contains (cur, &r))
      break;
    if (swfdec_rectangle_contains (&r, cur)) {
      *cur = r;
      swfdec_rectangle_union (&player->invalid_extents, &player->invalid_extents, &r);
    }
  }
  if (i == player->invalidations->len) {
    g_array_append_val (player->invalidations, r);
    swfdec_rectangle_union (&player->invalid_extents, &player->invalid_extents, &r);
  }
  SWFDEC_DEBUG ("toplevel invalidation of %g %g  %g %g - invalid region now %d %d  %d %d (%u subregions)",
      rect->x0, rect->y0, rect->x1, rect->y1,
      player->invalid_extents.x, player->invalid_extents.y, 
      player->invalid_extents.x + player->invalid_extents.width,
      player->invalid_extents.y + player->invalid_extents.height,
      player->invalidations->len);
}

/**
 * swfdec_player_get_level:
 * @player: a #SwfdecPlayer
 * @name: name of the level to request
 * @create: resource to create the movie with if it doesn't exist
 *
 * This function is used to look up root movies in the given @player. The 
 * algorithm used is like this: First, check that @name actually references a
 * root level movie. If it does not, return %NULL. If the movie for the given 
 * level already exists, return it. If it does not, create it when @create was 
 * set to %TRUE and return the newly created movie. Otherwise return %NULL.
 *
 * Returns: the #SwfdecMovie referenced by the given @name or %NULL if no such
 *          movie exists. Note that if a new movie is created, it will not be
 *          fully initialized (yes, this function sucks).
 **/
SwfdecSpriteMovie *
swfdec_player_get_level (SwfdecPlayer *player, const char *name, SwfdecResource *create)
{
  SwfdecSpriteMovie *movie;
  GList *walk;
  const char *s;
  char *end;
  int depth;
  gulong l;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  /* check name starts with "_level" */
  if (swfdec_strncmp (SWFDEC_AS_CONTEXT (player)->version, name, "_level", 6) != 0)
    return NULL;
  name += 6;
  /* extract depth from rest string (or fail if it's not a depth) */
  errno = 0;
  l = strtoul (name, &end, 10);
  if (errno != 0 || *end != 0 || l > G_MAXINT)
    return NULL;
  depth = l - 16384;
  /* find movie */
  for (walk = player->roots; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->depth < depth)
      continue;
    if (cur->depth == depth)
      return SWFDEC_SPRITE_MOVIE (cur);
    break;
  }
  /* bail if create isn't set*/
  if (create == NULL)
    return NULL;
  /* create new root movie */
  s = swfdec_as_context_give_string (SWFDEC_AS_CONTEXT (player), g_strdup_printf ("_level%lu", l));
  movie = SWFDEC_SPRITE_MOVIE (swfdec_movie_new (player, depth, NULL, create, NULL, s));
  SWFDEC_MOVIE (movie)->name = SWFDEC_AS_STR_EMPTY;
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
swfdec_player_load (SwfdecPlayer *player, const char *url, 
    SwfdecLoaderRequest request, SwfdecBuffer *buffer)
{
  SwfdecAsContext *cx;
  SwfdecSecurity *sec;
  SwfdecURL *full;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  g_assert (player->resource);
  /* create absolute url first */
  full = swfdec_url_new_relative (swfdec_loader_get_url (player->resource->loader), url);
  /* figure out the right security object (FIXME: let the person loading it provide it?) */
  cx = SWFDEC_AS_CONTEXT (player);
  if (cx->frame) {
    sec = cx->frame->security;
  } else {
    g_warning ("swfdec_player_load() should only be called from scripts");
    sec = SWFDEC_SECURITY (player->resource);
  }
  if (!swfdec_security_allow_url (sec, full)) {
    SWFDEC_ERROR ("not allowing access to %s", url);
    return NULL;
  }

  if (buffer) {
    return swfdec_loader_load (player->resource->loader, url, request, 
	(const char *) buffer->data, buffer->length);
  } else {
    return swfdec_loader_load (player->resource->loader, url, request, NULL, 0);
  }
}

static gboolean
is_ascii (const char *s)
{
  while (*s) {
    if (*s & 0x80)
      return FALSE;
    s++;
  }
  return TRUE;
}

/**
 * swfdec_player_fscommand:
 * @player: a #SwfdecPlayer
 * @command: the command to parse
 * @value: the value passed to the command
 *
 * Checks if @command is an FSCommand and if so, emits the 
 * SwfdecPlayer::fscommand signal. 
 *
 * Returns: %TRUE if an fscommand was found and the signal emitted, %FALSE 
 *          otherwise.
 **/
gboolean
swfdec_player_fscommand (SwfdecPlayer *player, const char *command, const char *value)
{
  char *real_command;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (command != NULL, FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  if (g_ascii_strncasecmp (command, "FSCommand:", 10) != 0)
    return FALSE;

  command += 10;
  if (!is_ascii (command)) {
    SWFDEC_ERROR ("command \"%s\" are not ascii, skipping fscommand", command);
    return TRUE;
  }
  real_command = g_ascii_strdown (command, -1);
  g_signal_emit (player, signals[FSCOMMAND], 0, real_command, value);
  g_free (real_command);
  return TRUE;
}

void
swfdec_player_launch (SwfdecPlayer *player, SwfdecLoaderRequest request, const char *url, 
    const char *target, SwfdecBuffer *data)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (url != NULL);
  g_return_if_fail (target != NULL);

  if (!g_ascii_strncasecmp (url, "FSCommand:", strlen ("FSCommand:"))) {
    const char *command = url + strlen ("FSCommand:");
    g_signal_emit (player, signals[FSCOMMAND], 0, command, target);
    return;
  }
  g_signal_emit (player, signals[LAUNCH], 0, request, url, target, data);
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
    swfdec_sprite_movie_init_context (player, version);
    swfdec_video_movie_init_context (player, version);
    swfdec_net_connection_init_context (player, version);
    swfdec_net_stream_init_context (player, version);

    swfdec_as_context_run_init_script (context, swfdec_initialize, 
	sizeof (swfdec_initialize), 8);

    if (context->state == SWFDEC_AS_CONTEXT_NEW) {
      context->state = SWFDEC_AS_CONTEXT_RUNNING;
      swfdec_as_object_set_constructor (player->roots->data, player->MovieClip);
    }
  }
  SWFDEC_INFO ("initializing player to size %ux%u", width, height);
  player->rate = rate;
  player->width = width;
  player->height = height;
  player->internal_width = player->stage_width >= 0 ? (guint) player->stage_width : player->width;
  player->internal_height = player->stage_height >= 0 ? (guint) player->stage_height : player->height;
  player->initialized = TRUE;
  if (rate) {
    player->iterate_timeout.timestamp = player->time;
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
 * @debugger: %NULL or a #SwfdecAsDebugger to use for debugging this player.
 *
 * Creates a new player.
 * This function calls swfdec_init () for you if it wasn't called before.
 *
 * Returns: The new player
 **/
SwfdecPlayer *
swfdec_player_new (SwfdecAsDebugger *debugger)
{
  SwfdecPlayer *player;

  swfdec_init ();
  player = g_object_new (SWFDEC_TYPE_PLAYER, "debugger", debugger, NULL);

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
 **/
void
swfdec_player_set_loader_with_variables (SwfdecPlayer *player, SwfdecLoader *loader,
    const char *variables)
{
  SwfdecMovie *movie;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (player->resource == NULL);
  g_return_if_fail (SWFDEC_IS_LOADER (loader));

  player->resource = swfdec_resource_new (loader, variables);
  movie = swfdec_movie_new (player, -16384, NULL, player->resource, NULL, SWFDEC_AS_STR__level0);
  movie->name = SWFDEC_AS_STR_EMPTY;
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

  loader = swfdec_file_loader_new (filename);
  player = swfdec_player_new (NULL);
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
 * Returns: %TRUE if the mouse event was handled. %FALSE to propagate the event
 *          further. A mouse event may not be handled if the user clicked on a 
 *          translucent area.
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
 * swfdec_player_key_press:
 * @player: a #SwfdecPlayer
 * @keycode: the key that was pressed
 * @character: UCS4 of the character that was inserted or 0 if none
 *
 * Call this function to make the @player react to a key press. Be sure to
 * check that keycode transformations are done correctly. For a list of 
 * keycodes see FIXME.
 *
 * Returns: %TRUE if the key press was handled by the @player, %FALSE if it
 *          should be propagated further
 **/
gboolean
swfdec_player_key_press (SwfdecPlayer *player, guint keycode, guint character)
{
  gboolean ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (keycode < 256, FALSE);

  g_signal_emit (player, signals[HANDLE_KEY], 0, keycode, character, TRUE, &ret);

  return ret;
}

/**
 * swfdec_player_key_release:
 * @player: a #SwfdecPlayer
 * @keycode: the key that was released
 * @character: UCS4 of the character that was inserted or 0 if none
 *
 * Call this function to make the @player react to a key being released. See
 * swfdec_player_key_press() for details.
 *
 * Returns: %TRUE if the key press was handled by the @player, %FALSE if it
 *          should be propagated further
 **/
gboolean
swfdec_player_key_release (SwfdecPlayer *player, guint keycode, guint character)
{
  gboolean ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (keycode < 256, FALSE);

  g_signal_emit (player, signals[HANDLE_KEY], 0, keycode, character, FALSE, &ret);

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
    swfdec_movie_render (walk->data, cr, &trans, &real);
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
swfdec_player_advance (SwfdecPlayer *player, gulong msecs)
{
  guint frames;
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

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
 * Returns: number of milliseconds until next event or -1 if no outstanding event
 **/
glong
swfdec_player_get_next_event (SwfdecPlayer *player)
{
  SwfdecTick tick;
  guint ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);

  tick = swfdec_player_get_next_event_time (player);
  if (tick == G_MAXUINT64)
    return -1;
  /* round up to full msecs */
  ret = SWFDEC_TICKS_TO_MSECS (tick + SWFDEC_TICKS_PER_SECOND / 1000 - 1); 

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
swfdec_player_update_size (gpointer playerp, gpointer unused)
{
  SwfdecPlayer *player = playerp;
  guint width, height;

  /* FIXME: only update if not fullscreen */
  width = player->stage_width >=0 ? (guint) player->stage_width : player->width;
  height = player->stage_height >=0 ? (guint) player->stage_height : player->height;
  /* only broadcast once */
  if (width == player->internal_width && height == player->internal_height)
    return;

  player->internal_width = width;
  player->internal_height = height;
  if (player->scale_mode == SWFDEC_SCALE_NONE)
    swfdec_player_broadcast (player, SWFDEC_AS_STR_Stage, SWFDEC_AS_STR_onResize);
}

/**
 * swfdec_player_set_size:
 * @player: a #SwfdecPlayer
 * @width: desired width of the movie or -1 for default
 * @height: desired height of the movie or -1 for default
 *
 * Sets the image size to the given values. The image size is what the area that
 * the @player will render and advocate with scripts.
 **/
void
swfdec_player_set_size (SwfdecPlayer *player, int width, int height)
{
  gboolean changed = FALSE;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

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
  if (changed)
    swfdec_player_add_external_action (player, player, swfdec_player_update_size, NULL);
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
  player->bgcolor = color;
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

