/* Swfedit
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

#include "swfedit_list.h"

G_DEFINE_TYPE (SwfeditList, swfedit_list, SWFEDIT_TYPE_TOKEN)

static void
swfedit_list_dispose (GObject *object)
{
  //SwfeditList *list = SWFEDIT_LIST (object);

  G_OBJECT_CLASS (swfedit_list_parent_class)->dispose (object);
}

static void
swfedit_list_changed (SwfeditToken *token, guint i)
{
  guint j;
  SwfeditList *list = SWFEDIT_LIST (token);
  SwfeditTokenEntry *entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);

  /* update visibility */
  for (j = i + 1; j % list->n_defs != 0; j++) {
    if (list->def[j].n_items == (j % list->n_defs) + 1) {
      swfedit_token_set_visible (token, j, entry->value != NULL);
    }
  }
  /* update length */
  j = list->def[i % list->n_defs].n_items;
  if (j != 0) {
    SwfeditTokenEntry *entry = &g_array_index (token->tokens, 
	SwfeditTokenEntry, j - 1);
    if (entry->type == SWFEDIT_TOKEN_UINT32) {
      SwfdecOut *out = swfdec_out_open ();
      SwfdecBuffer *buffer;
      swfedit_tag_write_token (token, out, i);
      buffer = swfdec_out_close (out);
      if (entry->value != GUINT_TO_POINTER (buffer->length)) {
	swfedit_token_set (token, i / list->n_defs * list->n_defs + j - 1, 
	    GUINT_TO_POINTER (buffer->length));
      }
      swfdec_buffer_unref (buffer);
    }
  }
  /* maybe add items */
  if (i == token->tokens->len - 1) {
    for (j = 0; j < list->n_defs; j++) {
      const SwfeditTagDefinition *def = &list->def[(j + 1) % list->n_defs];
      swfedit_tag_add_token (SWFEDIT_TOKEN (list), def->name, def->type, def->hint);
    }
  }
}

static void
swfedit_list_class_init (SwfeditListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfeditTokenClass *token_class = SWFEDIT_TOKEN_CLASS (klass);

  object_class->dispose = swfedit_list_dispose;

  token_class->changed = swfedit_list_changed;
}

static void
swfedit_list_init (SwfeditList *list)
{
}

static SwfeditList *
swfedit_list_new_internal (const SwfeditTagDefinition *def)
{
  SwfeditList *list;

  list = g_object_new (SWFEDIT_TYPE_LIST, NULL);
  list->def = def;
  for (; def->name; def++)
    list->n_defs++;

  return list;
}

SwfeditList *
swfedit_list_new (const SwfeditTagDefinition *def)
{
  SwfeditList *list;

  g_return_val_if_fail (def != NULL, NULL);

  list = swfedit_list_new_internal (def);
  swfedit_tag_add_token (SWFEDIT_TOKEN (list), def->name, def->type, def->hint);

  return list;
}

SwfeditList *
swfedit_list_new_read (SwfeditToken *parent, SwfdecBits *bits, const SwfeditTagDefinition *def)
{
  SwfeditList *list;
  SwfeditTokenEntry *entry;
  guint offset;

  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (def != NULL, NULL);

  list = swfedit_list_new_internal (def);
  SWFEDIT_TOKEN (list)->parent = parent;
  offset = 0;
  while (TRUE) {
    def = list->def;
    swfedit_tag_read_tag (SWFEDIT_TOKEN (list), bits, def);
    entry = &g_array_index (SWFEDIT_TOKEN (list)->tokens, SwfeditTokenEntry,
	SWFEDIT_TOKEN (list)->tokens->len - 1);
    if (GPOINTER_TO_UINT (entry->value) == 0)
      break;

    def++;
    for (;def->name != NULL; def++) {
      SwfeditTagDefinition def2 = *def;
      if (def2.n_items)
	def2.n_items += offset;
      swfedit_tag_read_tag (SWFEDIT_TOKEN (list), bits, &def2);
    }
    offset += list->n_defs;
  }
  return list;
}

