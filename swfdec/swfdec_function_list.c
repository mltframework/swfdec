/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_function_list.h"

typedef struct _SwfdecFunctionListEntry SwfdecFunctionListEntry;
struct _SwfdecFunctionListEntry {
  GFunc			func;
  gpointer		data;
  GDestroyNotify	destroy;
};

void
swfdec_function_list_clear (SwfdecFunctionList *list)
{
  GList *walk;

  g_return_if_fail (list != NULL);

  for (walk = list->list; walk; walk = walk->next) {
    SwfdecFunctionListEntry *entry = walk->data;
    if (entry->destroy)
      entry->destroy (entry->data);
    g_slice_free (SwfdecFunctionListEntry, entry);
  }
  g_list_free (list->list);
  list->list = NULL;
}

void
swfdec_function_list_add (SwfdecFunctionList *list, GFunc func,
    gpointer data, GDestroyNotify destroy)
{
  SwfdecFunctionListEntry *entry;

  g_return_if_fail (list != NULL);
  g_return_if_fail (func);

  entry = g_slice_new (SwfdecFunctionListEntry);
  entry->func = func;
  entry->data = data;
  entry->destroy = destroy;

  list->list = g_list_append (list->list, entry);
}

static int
swfdec_function_list_entry_compare (gconstpointer a, gconstpointer b)
{
  a = ((const SwfdecFunctionListEntry *) a)->data;
  b = ((const SwfdecFunctionListEntry *) b)->data;

  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

void
swfdec_function_list_remove (SwfdecFunctionList *list, gpointer data)
{
  SwfdecFunctionListEntry entry = { NULL, data, NULL };
  SwfdecFunctionListEntry *e;
  GList *node;

  g_return_if_fail (list != NULL);

  node = g_list_find_custom (list->list, &entry,
      swfdec_function_list_entry_compare);
  e = node->data;
  if (e->destroy)
    e->destroy (data);
  g_slice_free (SwfdecFunctionListEntry, e);
  list->list = g_list_delete_link (list->list, node);
}

void
swfdec_function_list_execute (SwfdecFunctionList *list, gpointer data)
{
  SwfdecFunctionListEntry *entry;
  GList *walk;

  g_return_if_fail (list != NULL);

  for (walk = list->list; walk; walk = walk->next) {
    entry = walk->data;
    entry->func (entry->data, data);
  }
}

void
swfdec_function_list_execute_and_clear (SwfdecFunctionList *list, gpointer data)
{
  SwfdecFunctionListEntry *entry;
  GList *old, *walk;

  g_return_if_fail (list != NULL);

  old = list->list;
  list->list = NULL;
  for (walk = old; walk; walk = walk->next) {
    entry = walk->data;
    entry->func (entry->data, data);
    if (entry->destroy)
      entry->destroy (entry->data);
    g_slice_free (SwfdecFunctionListEntry, entry);
  }
  g_list_free (old);
}

