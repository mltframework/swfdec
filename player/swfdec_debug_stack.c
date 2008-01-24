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

#include <swfdec/swfdec.h>
#include "swfdec_debug_stack.h"

G_DEFINE_TYPE (SwfdecDebugStack, swfdec_debug_stack, GTK_TYPE_TREE_VIEW)

enum {
  COLUMN_LINE,
  COLUMN_TYPE,
  COLUMN_CONTENT,
  N_COLUMNS
};

static const char *
swfdec_get_value_type (SwfdecAsContext *cx, SwfdecAsValue *value)
{
  switch (value->type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
      return "undefined";
    case SWFDEC_AS_TYPE_NULL:
      return "null";
    case SWFDEC_AS_TYPE_NUMBER:
      return "Number";
    case SWFDEC_AS_TYPE_BOOLEAN:
      return "Boolean";
    case SWFDEC_AS_TYPE_STRING:
      return "String";
    case SWFDEC_AS_TYPE_OBJECT:
      /* FIXME: improve */
      return "Object";
    default:
      g_assert_not_reached ();
      return NULL;
  }
}

static void
swfdec_debug_stack_set_model (SwfdecDebugStack *debug)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;
  GtkListStore *store = gtk_list_store_new (N_COLUMNS, G_TYPE_UINT, 
      G_TYPE_STRING, G_TYPE_STRING);
  GtkTreeIter iter;
  char *s;

  context= SWFDEC_AS_CONTEXT (debug->manager->player);
  frame = swfdec_as_context_get_frame (context);
  if (frame) {
    SwfdecAsStackIterator siter;
    SwfdecAsValue *val;
    guint i = 0;
    for (val = swfdec_as_stack_iterator_init (&siter, frame); val;
	val = swfdec_as_stack_iterator_next (&siter)) {
      s = swfdec_as_value_to_debug (val);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, COLUMN_LINE, ++i, 
	COLUMN_TYPE, swfdec_get_value_type (SWFDEC_AS_CONTEXT (debug->manager->player), val),
	COLUMN_CONTENT, s, -1);
      g_free (s);
    }
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (debug), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
update_stack_cb (SwfdecPlayerManager *manager, GParamSpec *pspec, SwfdecDebugStack *debug)
{
  if (swfdec_player_manager_get_interrupted (manager))
    swfdec_debug_stack_set_model (debug);
  else
    gtk_tree_view_set_model (GTK_TREE_VIEW (debug), NULL);
}

static void
swfdec_debug_stack_set_manager (SwfdecDebugStack *debug, SwfdecPlayerManager *manager)
{
  if (debug->manager) {
    g_signal_handlers_disconnect_by_func (debug->manager, update_stack_cb, debug);
    g_object_unref (debug->manager);
  }
  debug->manager = manager;
  if (manager) {
    g_object_ref (debug->manager);
    g_signal_connect (manager, "notify::interrupted", G_CALLBACK (update_stack_cb), debug);
    update_stack_cb (manager, NULL, debug);
  } else {
    gtk_tree_view_set_model (GTK_TREE_VIEW (debug), NULL);
  }
}

static void
swfdec_debug_stack_dispose (GObject *object)
{
  SwfdecDebugStack *debug = SWFDEC_DEBUG_STACK (object);

  swfdec_debug_stack_set_manager (debug, NULL);

  G_OBJECT_CLASS (swfdec_debug_stack_parent_class)->dispose (object);
}

static void
swfdec_debug_stack_class_init (SwfdecDebugStackClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_debug_stack_dispose;
}

static void
swfdec_debug_stack_init (SwfdecDebugStack * debug)
{
}

static void
swfdec_debug_stack_add_columns (GtkTreeView *treeview)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Depth", renderer,
    "text", COLUMN_LINE, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_LINE);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Type", renderer,
    "text", COLUMN_TYPE, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_TYPE);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Content", renderer,
    "text", COLUMN_CONTENT, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_CONTENT);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);
}

GtkWidget *
swfdec_debug_stack_new (SwfdecPlayerManager *manager)
{
  SwfdecDebugStack *debug;
  
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), NULL);

  debug = g_object_new (SWFDEC_TYPE_DEBUG_STACK, 0);
  swfdec_debug_stack_add_columns (GTK_TREE_VIEW (debug));
  swfdec_debug_stack_set_manager (debug, manager);

  return GTK_WIDGET (debug);
}

