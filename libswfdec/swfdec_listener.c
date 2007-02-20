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
#include "js/jsapi.h"
#include "js/jsfun.h"
#include "js/jsinterp.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

typedef struct {
  JSObject *	object;			/* the object we care about or NULL if empty */
  gboolean	blocked :1;		/* TRUE if may not be removed */
  gboolean	removed :1;		/* TRUE if was removed but is blocked */
} SwfdecListenerEntry;

struct _SwfdecListener {
  SwfdecPlayer *	player;
  /* we can't use GArray here because it reallocated below us, which JS_AddRoot doesn't like */
  SwfdecListenerEntry * entries;	/* all allocated entries */
  guint			n_entries;	/* number of allocated entries */
};

SwfdecListener *
swfdec_listener_new (SwfdecPlayer *player)
{
  SwfdecListener *listener = g_new0 (SwfdecListener, 1);

  listener->player = player;
  listener->entries = NULL;
  listener->n_entries = 0;

  return listener;
}

void
swfdec_listener_free (SwfdecListener *listener)
{
  guint i;
  JSContext *cx;

  g_return_if_fail (listener != NULL);

  cx = listener->player->jscx;
  for (i = 0; i < listener->n_entries; i++) {
    JS_RemoveRoot (cx, &listener->entries[i].object);
  }
  g_free (listener->entries);
  swfdec_player_remove_all_actions (listener->player, listener);
  g_free (listener);
}

gboolean
swfdec_listener_add (SwfdecListener *listener, JSObject *obj)
{
  guint found, i;

  g_return_val_if_fail (listener != NULL, FALSE);
  g_return_val_if_fail (obj != NULL, FALSE);

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
    JSContext *cx = listener->player->jscx;

    for (i = 0; i < listener->n_entries; i++) {
      JS_RemoveRoot (cx, &listener->entries[i].object);
    }
    mem = g_try_realloc (listener->entries, sizeof (SwfdecListenerEntry) * new_len);
    if (mem == NULL)
      return FALSE;
    listener->entries = mem;
    for (i = listener->n_entries; i < new_len; i++) {
      listener->entries[i].object = NULL;
      listener->entries[i].blocked = 0;
      listener->entries[i].removed = 0;
      if (!JS_AddRoot (cx, &listener->entries[i].object)) {
	listener->n_entries = i;
	return FALSE;
      }
    }
    listener->n_entries = new_len;
  }
  g_assert (listener->entries[found].object == NULL);
  listener->entries[found].object = obj;
  return TRUE;
}

void
swfdec_listener_remove (SwfdecListener *listener, JSObject *obj)
{
  guint i;

  g_return_if_fail (listener != NULL);
  g_return_if_fail (obj != NULL);

  for (i = 0; i < listener->n_entries; i++) {
    if (listener->entries[i].object == obj) {
      if (listener->entries[i].blocked) {
	listener->entries[i].removed = TRUE;
      } else {
	listener->entries[i].object = NULL;
      }
      return;
    }
  }
}

static void
swfdec_listener_do_execute (gpointer listenerp, gpointer event_name)
{
  guint i;
  SwfdecListener *listener = listenerp;
  JSContext *cx = listener->player->jscx;

  for (i = 0; i < listener->n_entries; i++) {
    if (listener->entries[i].blocked) {
      jsval fun;
      JSObject *obj = listener->entries[i].object;
      if (listener->entries[i].removed) {
	listener->entries[i].object = NULL;
	listener->entries[i].removed = FALSE;
      }
      listener->entries[i].blocked = FALSE;
      if (!JS_GetProperty (cx, obj, event_name, &fun))
	continue;
      if (!JSVAL_IS_OBJECT (fun) || fun == JSVAL_NULL ||
	  JS_GetClass (JSVAL_TO_OBJECT (fun)) != &js_FunctionClass) {
	SWFDEC_INFO ("object has no handler for %s event", (char *) event_name);
	continue;
      }
      js_InternalCall (cx, obj, fun, 0, NULL, &fun);
    }
  }
  g_free (event_name);
}

void
swfdec_listener_execute	(SwfdecListener *listener, const char *event_name)
{
  gboolean found = FALSE;
  guint i;

  g_return_if_fail (listener != NULL);
  g_return_if_fail (event_name != NULL);

  for (i = 0; i < listener->n_entries; i++) {
    g_assert (!listener->entries[i].blocked); /* ensure this happens only once */
    if (listener->entries[i].object) {
      listener->entries[i].blocked = TRUE;
      found = TRUE;
    }
  }
  if (found)
    swfdec_player_add_action (listener->player, listener, 
	swfdec_listener_do_execute, g_strdup (event_name));
}

