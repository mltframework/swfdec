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

#include "swfdec_debug_scripts.h"
#include <swfdec/swfdec_script_internal.h>

G_DEFINE_TYPE (SwfdecDebugScripts, swfdec_debug_scripts, GTK_TYPE_TREE_VIEW)

enum {
  COLUMN_SCRIPT,
  COLUMN_NAME,
  N_COLUMNS
};

static void
add_row (gpointer scriptp, gpointer storep)
{
  SwfdecDebuggerScript *script = scriptp;
  GtkListStore *store = storep;
  GtkTreeIter iter;

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, COLUMN_SCRIPT, script, 
      COLUMN_NAME, script->script->name, -1);
}

static void
swfdec_debug_scripts_set_model (SwfdecDebugScripts *debug)
{
  GtkListStore *store = gtk_list_store_new (N_COLUMNS, G_TYPE_POINTER, G_TYPE_STRING);

  swfdec_debugger_foreach_script (debug->debugger, add_row, store);
  gtk_tree_view_set_model (GTK_TREE_VIEW (debug), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
swfdec_debug_scripts_add_script_cb (SwfdecDebugger *debugger, 
    SwfdecDebuggerScript *script, SwfdecDebugScripts *debug)
{
  add_row (script, gtk_tree_view_get_model (GTK_TREE_VIEW (debug)));
}

static void
swfdec_debug_scripts_remove_script_cb (SwfdecDebugger *debugger, 
    SwfdecDebuggerScript *script, SwfdecDebugScripts *debug)
{
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (debug));
  GtkTreeIter iter;

  gtk_tree_model_get_iter_first (model, &iter);
  do {
    gpointer p;
    gtk_tree_model_get (model, &iter, COLUMN_SCRIPT, &p, -1);
    if (p == script) {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      return;
    }
  } while (gtk_tree_model_iter_next (model, &iter));
  g_assert_not_reached ();
}

static void
swfdec_debug_scripts_set_debugger (SwfdecDebugScripts *debug, SwfdecDebugger *debugger)
{
  if (debug->debugger) {
    g_signal_handlers_disconnect_by_func (debug->debugger, swfdec_debug_scripts_remove_script_cb, debug);
    g_signal_handlers_disconnect_by_func (debug->debugger, swfdec_debug_scripts_add_script_cb, debug);
  }
  debug->debugger = debugger;
  if (debugger) {
    g_signal_connect (debugger, "script-added", G_CALLBACK (swfdec_debug_scripts_add_script_cb), debug);
    g_signal_connect (debugger, "script-removed", G_CALLBACK (swfdec_debug_scripts_remove_script_cb), debug);
    swfdec_debug_scripts_set_model (debug);
  } else {
    gtk_tree_view_set_model (GTK_TREE_VIEW (debug), NULL);
  }
}

static void
swfdec_debug_scripts_dispose (GObject *object)
{
  SwfdecDebugScripts *debug = SWFDEC_DEBUG_SCRIPTS (object);

  swfdec_debug_scripts_set_debugger (debug, NULL);

  G_OBJECT_CLASS (swfdec_debug_scripts_parent_class)->dispose (object);
}

static void
swfdec_debug_scripts_class_init (SwfdecDebugScriptsClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_debug_scripts_dispose;
}

static void
swfdec_debug_scripts_init (SwfdecDebugScripts * debug)
{
}

static void
swfdec_debug_scripts_add_columns (GtkTreeView *treeview)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Script", renderer,
    "text", COLUMN_NAME, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);
}

GtkWidget *
swfdec_debug_scripts_new (SwfdecDebugger *debugger)
{
  SwfdecDebugScripts *debug;
  
  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), NULL);

  debug = g_object_new (SWFDEC_TYPE_DEBUG_SCRIPTS, 0);
  swfdec_debug_scripts_add_columns (GTK_TREE_VIEW (debug));
  swfdec_debug_scripts_set_debugger (debug, debugger);

  return GTK_WIDGET (debug);
}

