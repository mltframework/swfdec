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

#include "swfdec_debug_script.h"

G_DEFINE_TYPE (SwfdecDebugScript, swfdec_debug_script, GTK_TYPE_TREE_VIEW)

enum {
  COLUMN_COMMAND,
  COLUMN_BREAKPOINT,
  COLUMN_DESC,
  N_COLUMNS
};

static void
swfdec_debug_script_set_debugger (SwfdecDebugScript *debug, SwfdecDebugger *debugger)
{
  debug->debugger = debugger;
  swfdec_debug_script_set_script (debug, NULL);
}

static void
swfdec_debug_script_dispose (GObject *object)
{
  SwfdecDebugScript *debug = SWFDEC_DEBUG_SCRIPT (object);

  swfdec_debug_script_set_debugger (debug, NULL);

  G_OBJECT_CLASS (swfdec_debug_script_parent_class)->dispose (object);
}

static void
swfdec_debug_script_class_init (SwfdecDebugScriptClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_debug_script_dispose;
}

static void
swfdec_debug_script_init (SwfdecDebugScript * debug)
{
}

static void
swfdec_debug_script_add_columns (GtkTreeView *treeview)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Command", renderer,
    "text", COLUMN_DESC, NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_DESC);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (treeview, column);
}

GtkWidget *
swfdec_debug_script_new (SwfdecPlayer *player)
{
  SwfdecDebugScript *debug;
  
  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);

  debug = g_object_new (SWFDEC_TYPE_DEBUG_SCRIPT, 0);
  swfdec_debug_script_add_columns (GTK_TREE_VIEW (debug));
  swfdec_debug_script_set_debugger (debug, swfdec_debugger_get (player));

  return GTK_WIDGET (debug);
}

static void
swfdec_debug_script_set_model (SwfdecDebugScript *debug)
{
  guint i;
  GtkListStore *store = gtk_list_store_new (N_COLUMNS, G_TYPE_POINTER, 
      G_TYPE_BOOLEAN, G_TYPE_STRING);

  for (i = 0; i < debug->script->n_commands; i++) {
    SwfdecDebuggerCommand *command = &debug->script->commands[i];
    GtkTreeIter iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, COLUMN_COMMAND, command,
	COLUMN_BREAKPOINT, FALSE, COLUMN_DESC, command->description, -1);
  }

  gtk_tree_view_set_model (GTK_TREE_VIEW (debug), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
swfdec_debug_script_set_script (SwfdecDebugScript *debug, SwfdecDebuggerScript *dscript)
{
  g_return_if_fail (SWFDEC_IS_DEBUG_SCRIPT (debug));

  if (debug->debugger == NULL)
    return;
  debug->script = dscript;
  if (dscript) {
    swfdec_debug_script_set_model (debug);
  } else {
    gtk_tree_view_set_model (GTK_TREE_VIEW (debug), NULL);
  }
}

