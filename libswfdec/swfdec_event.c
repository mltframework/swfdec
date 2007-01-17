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
#include <js/jsapi.h>
#include "swfdec_event.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"
#include "swfdec_script.h"

typedef struct _SwfdecEvent SwfdecEvent;

struct _SwfdecEvent {
  unsigned int	conditions;
  guint8	key;
  SwfdecScript *script;
};

struct _SwfdecEventList {
  SwfdecPlayer *	player;
  guint			refcount;
  GArray *		events;
};


SwfdecEventList *
swfdec_event_list_new (SwfdecPlayer *player)
{
  SwfdecEventList *list;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  list = g_new0 (SwfdecEventList, 1);
  list->player = player;
  list->refcount = 1;
  list->events = g_array_new (FALSE, FALSE, sizeof (SwfdecEvent));

  return list;
}

/* FIXME: this is a bit nasty because of modifying */
SwfdecEventList *
swfdec_event_list_copy (SwfdecEventList *list)
{
  g_return_val_if_fail (list != NULL, NULL);

  list->refcount++;

  return list;
}

void
swfdec_event_list_free (SwfdecEventList *list)
{
  unsigned int i;

  g_return_if_fail (list != NULL);

  list->refcount--;
  if (list->refcount > 0)
    return;

  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    swfdec_script_unref (event->script);
  }
  g_array_free (list->events, TRUE);
  g_free (list);
}

static const char *
swfdec_event_list_condition_name (unsigned int conditions)
{
  if (conditions & SWFDEC_EVENT_LOAD)
    return "Load";
  if (conditions & SWFDEC_EVENT_ENTER)
    return "Enter";
  if (conditions & SWFDEC_EVENT_UNLOAD)
    return "Unload";
  if (conditions & SWFDEC_EVENT_MOUSE_MOVE)
    return "MouseMove";
  if (conditions & SWFDEC_EVENT_MOUSE_DOWN)
    return "MouseDown";
  if (conditions & SWFDEC_EVENT_MOUSE_UP)
    return "MouseUp";
  if (conditions & SWFDEC_EVENT_KEY_UP)
    return "KeyUp";
  if (conditions & SWFDEC_EVENT_KEY_DOWN)
    return "KeyDown";
  if (conditions & SWFDEC_EVENT_DATA)
    return "Data";
  if (conditions & SWFDEC_EVENT_INITIALIZE)
    return "Initialize";
  if (conditions & SWFDEC_EVENT_PRESS)
    return "Press";
  if (conditions & SWFDEC_EVENT_RELEASE)
    return "Release";
  if (conditions & SWFDEC_EVENT_RELEASE_OUTSIDE)
    return "ReleaseOutside";
  if (conditions & SWFDEC_EVENT_ROLL_OVER)
    return "RollOver";
  if (conditions & SWFDEC_EVENT_ROLL_OUT)
    return "RollOut";
  if (conditions & SWFDEC_EVENT_DRAG_OVER)
    return "DragOver";
  if (conditions & SWFDEC_EVENT_DRAG_OUT)
    return "DragOut";
  if (conditions & SWFDEC_EVENT_KEY_PRESS)
    return "KeyPress";
  if (conditions & SWFDEC_EVENT_CONSTRUCT)
    return "Construct";
  return "No Event";
}

void
swfdec_event_list_parse (SwfdecEventList *list, SwfdecBits *bits, int version,
    unsigned int conditions, guint8 key, const char *description)
{
  SwfdecEvent event;
  char *name;

  g_return_if_fail (list != NULL);
  g_return_if_fail (list->refcount == 1);
  g_return_if_fail (description != NULL);

  event.conditions = conditions;
  event.key = key;
  name = g_strconcat (description, ".", 
      swfdec_event_list_condition_name (conditions), NULL);
  event.script = swfdec_script_new (bits, name, version);
  g_free (name);
  if (event.script) 
    g_array_append_val (list->events, event);
}

void
swfdec_event_list_execute (SwfdecEventList *list, SwfdecScriptable *scriptable, 
    unsigned int conditions, guint8 key)
{
  unsigned int i;

  g_return_if_fail (list != NULL);

  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    if ((event->conditions & conditions) &&
	event->key == key) {
      SWFDEC_LOG ("executing script for event %u on scriptable %p", conditions, scriptable);
      swfdec_script_execute (event->script, scriptable);
    }
  }
}

gboolean
swfdec_event_list_has_conditions (SwfdecEventList *list, 
    unsigned int conditions, guint8 key)
{
  unsigned int i;

  g_return_val_if_fail (list != NULL, FALSE);

  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    if ((event->conditions & conditions) &&
	event->key == key)
      return TRUE;
  }
  return FALSE;
}

