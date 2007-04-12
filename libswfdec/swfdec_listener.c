/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_listener.h"
#include "swfdec_as_context.h"
#include "swfdec_as_object.h"
#include "swfdec_debug.h"

typedef struct {
  SwfdecAsObject *	object;		/* the object we care about or NULL if empty */
  const char *		blocked_by;	/* string of event we're about to execute */
  gboolean		removed;   	/* TRUE if was removed but is blocked */
} SwfdecListenerEntry;

struct _SwfdecListener {
  SwfdecAsContext *	context;
  /* we can't use GArray here because it reallocated below us, which JS_AddRoot doesn't like */
  SwfdecListenerEntry * entries;	/* all allocated entries */
  guint			n_entries;	/* number of allocated entries */
};

SwfdecListener *
swfdec_listener_new (SwfdecAsContext *context)
{
  SwfdecListener *listener;
  
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  listener = g_new0 (SwfdecListener, 1);
  listener->context = context;
  listener->entries = NULL;
  listener->n_entries = 0;

  return listener;
}

void
swfdec_listener_free (SwfdecListener *listener)
{
  g_return_if_fail (listener != NULL);

  g_free (listener->entries);
  g_free (listener);
}

gboolean
swfdec_listener_add (SwfdecListener *listener, SwfdecAsObject *obj)
{
  guint found, i;

  g_return_val_if_fail (listener != NULL, FALSE);
  g_return_val_if_fail (SWFDEC_AS_OBJECT (obj), FALSE);

  found = listener->n_entries;
  for (i = 0; i < listener->n_entries; i++) {
    if (listener->entries[i].object == NULL && found >= listener->n_entries)
      found = i;
    else if (listener->entries[i].object == obj)
      return TRUE;
  }
  if (found >= listener->n_entries) {
    gpointer mem;
    guint new_len = listener->n_entries + 16;

    mem = g_try_realloc (listener->entries, sizeof (SwfdecListenerEntry) * new_len);
    if (mem == NULL)
      return FALSE;
    listener->entries = mem;
    listener->n_entries = new_len;
  }
  g_assert (listener->entries[found].object == NULL);
  listener->entries[found].object = obj;
  return TRUE;
}

void
swfdec_listener_remove (SwfdecListener *listener, SwfdecAsObject *obj)
{
  guint i;

  g_return_if_fail (listener != NULL);
  g_return_if_fail (obj != NULL);

  for (i = 0; i < listener->n_entries; i++) {
    if (listener->entries[i].object == obj) {
      if (listener->entries[i].blocked_by) {
	listener->entries[i].removed = TRUE;
      } else {
	listener->entries[i].object = NULL;
      }
      return;
    }
  }
}

void
swfdec_listener_execute	(SwfdecListener *listener, const char *event_name)
{
  guint i;

  g_return_if_fail (listener != NULL);
  g_return_if_fail (event_name != NULL);

  for (i = 0; i < listener->n_entries; i++) {
    g_assert (listener->entries[i].blocked_by == NULL); /* ensure this happens only once */
    if (listener->entries[i].object) {
      listener->entries[i].blocked_by = event_name;
    }
  }
  for (i = 0; i < listener->n_entries; i++) {
    if (listener->entries[i].blocked_by) {
      SwfdecAsObject *obj = listener->entries[i].object;
      const char *event = listener->entries[i].blocked_by;
      if (listener->entries[i].removed) {
	listener->entries[i].object = NULL;
	listener->entries[i].removed = FALSE;
      }
      listener->entries[i].blocked_by = NULL;
      swfdec_as_object_call (obj, event, 0, NULL, NULL);
    }
  }
}

void
swfdec_listener_mark (SwfdecListener *listener)
{
  guint i;

  g_return_if_fail (listener != NULL);

  for (i = 0; i < listener->n_entries; i++) {
    if (listener->entries[i].object) {
      swfdec_as_object_mark (listener->entries[i].object);
      if (listener->entries[i].blocked_by)
	swfdec_as_string_mark (listener->entries[i].blocked_by);
    }
  }
}

