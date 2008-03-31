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

#include "swfdec_actor.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_button_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sprite_movie.h"


G_DEFINE_ABSTRACT_TYPE (SwfdecActor, swfdec_actor, SWFDEC_TYPE_MOVIE)

static void
swfdec_actor_dispose (GObject *object)
{
  SwfdecActor *actor = SWFDEC_ACTOR (object);

  if (actor->events) {
    swfdec_event_list_free (actor->events);
    actor->events = NULL;
  }

  G_OBJECT_CLASS (swfdec_actor_parent_class)->dispose (object);
}

static gboolean
swfdec_actor_iterate_end (SwfdecActor *actor)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (actor);

  return movie->parent == NULL || 
	 movie->state < SWFDEC_MOVIE_STATE_REMOVED;
}

static gboolean
swfdec_actor_mouse_events (SwfdecActor *actor)
{
  SwfdecAsObject *object;

  /* root movies don't get event */
  if (SWFDEC_MOVIE (actor)->parent == NULL)
    return FALSE;
  /* look if we have a script that gets events */
  if (actor->events && swfdec_event_list_has_mouse_events (actor->events))
    return TRUE;
  /* otherwise, require at least one of the custom script handlers */
  object = SWFDEC_AS_OBJECT (actor);
  if (swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onRollOver) ||
      swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onRollOut) ||
      swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onDragOver) ||
      swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onDragOut) ||
      swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onPress) ||
      swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onRelease) ||
      swfdec_as_object_has_variable (object, SWFDEC_AS_STR_onReleaseOutside))
    return TRUE;
  return FALSE;
}

static void
swfdec_actor_mouse_in (SwfdecActor *actor)
{
  if (swfdec_player_is_mouse_pressed (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (actor)->context)))
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_DRAG_OVER);
  else
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_ROLL_OVER);
}

static void
swfdec_actor_mouse_out (SwfdecActor *actor)
{
  if (swfdec_player_is_mouse_pressed (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (actor)->context)))
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_DRAG_OUT);
  else
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_ROLL_OUT);
}

static void
swfdec_actor_mouse_press (SwfdecActor *actor, guint button)
{
  if (button != 0)
    return;
  swfdec_actor_queue_script (actor, SWFDEC_EVENT_PRESS);
}

static void
swfdec_actor_mouse_release (SwfdecActor *actor, guint button)
{
  SwfdecPlayer *player;
  
  if (button != 0)
    return;

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (actor)->context);
  if (player->priv->mouse_below == actor)
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_RELEASE);
  else
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_RELEASE_OUTSIDE);
}

static void
swfdec_actor_mouse_move (SwfdecActor *actor, double x, double y)
{
  /* nothing to do here, it's just there so we don't need to check for NULL */
}

static void
swfdec_actor_key_press (SwfdecActor *actor, guint keycode, guint character)
{
  swfdec_actor_queue_script (actor, SWFDEC_EVENT_KEY_DOWN);
}

static void
swfdec_actor_key_release (SwfdecActor *actor, guint keycode, guint character)
{
  swfdec_actor_queue_script (actor, SWFDEC_EVENT_KEY_UP);
}

static void
swfdec_actor_class_init (SwfdecActorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_actor_dispose;

  klass->iterate_end = swfdec_actor_iterate_end;
  klass->mouse_events = swfdec_actor_mouse_events;
  klass->mouse_in = swfdec_actor_mouse_in;
  klass->mouse_out = swfdec_actor_mouse_out;
  klass->mouse_press = swfdec_actor_mouse_press;
  klass->mouse_release = swfdec_actor_mouse_release;
  klass->mouse_move = swfdec_actor_mouse_move;
  klass->key_press = swfdec_actor_key_press;
  klass->key_release = swfdec_actor_key_release;
}

static void
swfdec_actor_init (SwfdecActor *actor)
{
}

static void
swfdec_sprite_movie_set_constructor (SwfdecSpriteMovie *movie)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecAsContext *context = SWFDEC_AS_OBJECT (movie)->context;
  SwfdecAsObject *constructor = NULL;

  g_assert (mov->resource != NULL);

  if (movie->sprite) {
    const char *name;

    name = swfdec_resource_get_export_name (mov->resource,
	SWFDEC_CHARACTER (movie->sprite));
    if (name != NULL) {
      name = swfdec_as_context_get_string (context, name);
      constructor = swfdec_player_get_export_class (SWFDEC_PLAYER (context),
	  name);
    }
  }
  if (constructor == NULL)
    constructor = mov->resource->sandbox->MovieClip;

  swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie), constructor);
}

void
swfdec_actor_execute (SwfdecActor *actor, SwfdecEventType condition)
{
  SwfdecAsObject *thisp;
  const char *name;
  guint version;

  g_return_if_fail (SWFDEC_IS_ACTOR (actor));

  version = swfdec_movie_get_version (SWFDEC_MOVIE (actor));

  if (SWFDEC_IS_BUTTON_MOVIE (actor)) {
    /* these conditions don't exist for buttons */
    if (condition == SWFDEC_EVENT_CONSTRUCT || condition < SWFDEC_EVENT_PRESS)
      return;
    thisp = SWFDEC_AS_OBJECT (SWFDEC_MOVIE (actor)->parent);
    if (version <= 5) {
      while (!SWFDEC_IS_SPRITE_MOVIE (thisp))
	thisp = SWFDEC_AS_OBJECT (SWFDEC_MOVIE (thisp)->parent);
    }
    g_assert (thisp);
  } else {
    thisp = SWFDEC_AS_OBJECT (actor);
  }

  /* special cases */
  if (condition == SWFDEC_EVENT_CONSTRUCT) {
    if (version <= 5)
      return;
    swfdec_sprite_movie_set_constructor (SWFDEC_SPRITE_MOVIE (actor));
  } else if (condition == SWFDEC_EVENT_ENTER) {
    if (SWFDEC_MOVIE (actor)->state >= SWFDEC_MOVIE_STATE_REMOVED)
      return;
  }

  swfdec_sandbox_use (SWFDEC_MOVIE (actor)->resource->sandbox);
  if (actor->events) {
    swfdec_event_list_execute (actor->events, thisp, condition, 0);
  }
  /* FIXME: how do we compute the version correctly here? */
  if (version > 5) {
    name = swfdec_event_type_get_name (condition);
    if (name != NULL) {
      swfdec_as_object_call (SWFDEC_AS_OBJECT (actor), name, 0, NULL, NULL);
    }
    if (condition == SWFDEC_EVENT_CONSTRUCT)
      swfdec_as_object_call (thisp, SWFDEC_AS_STR_constructor, 0, NULL, NULL);
  }
  swfdec_sandbox_unuse (SWFDEC_MOVIE (actor)->resource->sandbox);
}

/**
 * swfdec_actor_queue_script:
 * @movie: a #SwfdecMovie
 * @condition: the event that should happen
 *
 * Queues execution of all scripts associated with the given event.
 **/
void
swfdec_actor_queue_script (SwfdecActor *actor, SwfdecEventType condition)
{
  SwfdecPlayer *player;
  guint importance;
  
  g_return_if_fail (SWFDEC_IS_ACTOR (actor));

  if (!SWFDEC_IS_SPRITE_MOVIE (actor) && !SWFDEC_IS_BUTTON_MOVIE (actor))
    return;
  /* can happen for mouse/keyboard events on the initial movie */
  if (SWFDEC_MOVIE (actor)->resource->sandbox == NULL) {
    SWFDEC_INFO ("movie %s not yet initialized, skipping event", SWFDEC_MOVIE (actor)->name);
    return;
  }

  switch (condition) {
    case SWFDEC_EVENT_INITIALIZE:
      importance = 0;
      break;
    case SWFDEC_EVENT_CONSTRUCT:
      importance = 1;
      break;
    case SWFDEC_EVENT_LOAD:
    case SWFDEC_EVENT_ENTER:
    case SWFDEC_EVENT_UNLOAD:
    case SWFDEC_EVENT_MOUSE_MOVE:
    case SWFDEC_EVENT_MOUSE_DOWN:
    case SWFDEC_EVENT_MOUSE_UP:
    case SWFDEC_EVENT_KEY_UP:
    case SWFDEC_EVENT_KEY_DOWN:
    case SWFDEC_EVENT_DATA:
    case SWFDEC_EVENT_PRESS:
    case SWFDEC_EVENT_RELEASE:
    case SWFDEC_EVENT_RELEASE_OUTSIDE:
    case SWFDEC_EVENT_ROLL_OVER:
    case SWFDEC_EVENT_ROLL_OUT:
    case SWFDEC_EVENT_DRAG_OVER:
    case SWFDEC_EVENT_DRAG_OUT:
    case SWFDEC_EVENT_KEY_PRESS:
      importance = 2;
      break;
    default:
      g_return_if_reached ();
  }

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (actor)->context);
  swfdec_player_add_action (player, actor, condition, importance);
}

/**
 * swfdec_actor_get_mouse_events:
 * @movie: a #SwfdecActor
 *
 * Checks if this actor should respond to mouse events.
 *
 * Returns: %TRUE if this movie can receive mouse events
 **/
gboolean
swfdec_actor_get_mouse_events (SwfdecActor *actor)
{
  SwfdecActorClass *klass;

  g_return_val_if_fail (SWFDEC_IS_ACTOR (actor), FALSE);

  klass = SWFDEC_ACTOR_GET_CLASS (actor);
  if (klass->mouse_events)
    return klass->mouse_events (actor);
  else
    return FALSE;
}

