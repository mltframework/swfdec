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
#include "swfdec_event.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_script_internal.h"

typedef struct _SwfdecEvent SwfdecEvent;

struct _SwfdecEvent {
  guint	conditions;
  guint8	key;
  SwfdecScript *script;
};

struct _SwfdecEventList {
  guint			refcount;
  GArray *		events;
};

/**
 * swfdec_event_type_get_name:
 * @type: a #SwfdecEventType
 *
 * Gets the name for the event as a refcounted string or %NULL if the
 * given clip event has no associated event.
 *
 * Returns: The name of the event or %NULL if none.
 **/
const char *
swfdec_event_type_get_name (SwfdecEventType type)
{
  switch (type) {
    case SWFDEC_EVENT_LOAD:
      return SWFDEC_AS_STR_onLoad;
    case SWFDEC_EVENT_ENTER:
      return SWFDEC_AS_STR_onEnterFrame;
    case SWFDEC_EVENT_UNLOAD:
      return SWFDEC_AS_STR_onUnload;
    case SWFDEC_EVENT_MOUSE_MOVE:
      return SWFDEC_AS_STR_onMouseMove;
    case SWFDEC_EVENT_MOUSE_DOWN:
      return SWFDEC_AS_STR_onMouseDown;
    case SWFDEC_EVENT_MOUSE_UP:
      return SWFDEC_AS_STR_onMouseUp;
    case SWFDEC_EVENT_KEY_UP:
      return SWFDEC_AS_STR_onKeyUp;
    case SWFDEC_EVENT_KEY_DOWN:
      return SWFDEC_AS_STR_onKeyDown;
    case SWFDEC_EVENT_DATA:
      return SWFDEC_AS_STR_onData;
    case SWFDEC_EVENT_INITIALIZE:
      return NULL;
    case SWFDEC_EVENT_PRESS:
      return SWFDEC_AS_STR_onPress;
    case SWFDEC_EVENT_RELEASE:
      return SWFDEC_AS_STR_onRelease;
    case SWFDEC_EVENT_RELEASE_OUTSIDE:
      return SWFDEC_AS_STR_onReleaseOutside;
    case SWFDEC_EVENT_ROLL_OVER:
      return SWFDEC_AS_STR_onRollOver;
    case SWFDEC_EVENT_ROLL_OUT:
      return SWFDEC_AS_STR_onRollOut;
    case SWFDEC_EVENT_DRAG_OVER:
      return SWFDEC_AS_STR_onDragOver;
    case SWFDEC_EVENT_DRAG_OUT:
      return SWFDEC_AS_STR_onDragOut;
    case SWFDEC_EVENT_KEY_PRESS:
      return NULL;
    case SWFDEC_EVENT_CONSTRUCT:
      return SWFDEC_AS_STR_onConstruct;
    case SWFDEC_EVENT_CHANGED:
      return SWFDEC_AS_STR_onChanged;
    case SWFDEC_EVENT_SCROLL:
      return SWFDEC_AS_STR_onScroller;
    default:
      g_assert_not_reached ();
      return NULL;
  }
}

SwfdecEventList *
swfdec_event_list_new (void)
{
  SwfdecEventList *list;

  list = g_new0 (SwfdecEventList, 1);
  list->refcount = 1;
  list->events = g_array_new (FALSE, FALSE, sizeof (SwfdecEvent));

  return list;
}

/* FIXME: this is a bit nasty because of modifying */
SwfdecEventList *
swfdec_event_list_copy (SwfdecEventList *list)
{
  g_return_val_if_fail (list != NULL, NULL);
  g_return_val_if_fail (list->refcount > 0, NULL);

  list->refcount++;

  return list;
}

void
swfdec_event_list_free (SwfdecEventList *list)
{
  guint i;

  g_return_if_fail (list != NULL);
  g_return_if_fail (list->refcount > 0);

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

#define N_CONDITIONS 19
void
swfdec_event_list_parse (SwfdecEventList *list, SwfdecBits *bits, int version,
    guint conditions, guint8 key, const char *description)
{
  SwfdecEvent event;
  char *name;
  guint i;

  g_return_if_fail (list != NULL);
  g_return_if_fail (list->refcount == 1);
  g_return_if_fail (description != NULL);

  event.conditions = conditions & SWFDEC_EVENT_MASK;
  event.key = key;
  i = g_bit_nth_lsf (conditions, -1);
  name = g_strconcat (description, ".", i < N_CONDITIONS ? 
      swfdec_event_type_get_name (i) : "???", NULL);
  event.script = swfdec_script_new_from_bits (bits, name, version);
  g_free (name);
  if (event.script) 
    g_array_append_val (list->events, event);
}

void
swfdec_event_list_execute (SwfdecEventList *list, SwfdecAsObject *object, 
    guint condition, guint8 key)
{
  guint i;

  g_return_if_fail (list != NULL);
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (condition < N_CONDITIONS);

  condition = (1 << condition);
  /* FIXME: Do we execute all events if the event list is gone already? */
  /* need to ref here because followup code could free all references to the list */
  list = swfdec_event_list_copy (list);
  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    if ((event->conditions & condition) &&
	(condition != 1 << SWFDEC_EVENT_KEY_DOWN || event->key == key)) {
      SWFDEC_LOG ("executing script for event %u on scriptable %p", condition, object);
      swfdec_as_object_run (object, event->script);
    }
  }
  swfdec_event_list_free (list);
}

gboolean
swfdec_event_list_has_conditions (SwfdecEventList *list, SwfdecAsObject *object,
    guint condition, guint8 key)
{
  guint i;

  g_return_val_if_fail (list != NULL, FALSE);
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (condition < N_CONDITIONS, FALSE);

  condition = 1 << condition;
  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    if ((event->conditions & condition) &&
	event->key == key)
      return TRUE;
  }
  return FALSE;
}

#define MOUSE_EVENTS 0x1FC0
gboolean
swfdec_event_list_has_mouse_events (SwfdecEventList *list)
{
  guint i;

  g_return_val_if_fail (list != NULL, FALSE);

  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    if (event->conditions & MOUSE_EVENTS)
      return TRUE;
  }
  return FALSE;
}
