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

#include <gtk/gtk.h>
#include "swfedit_file.h"

static gboolean
open_window (char *filename)
{
  SwfeditFile *file;
  GtkWidget *window, *treeview;
  GError *error = NULL;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  file = swfedit_file_new (filename, &error);
  if (file == NULL) {
    g_printerr ("Error openeing file %s: %s\n", filename, error->message);
    g_error_free (error);
    return FALSE;
  }
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (file));
  gtk_container_add (GTK_CONTAINER (window), treeview);
  gtk_widget_show_all (window);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
    "text", SWFEDIT_COLUMN_NAME, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  column = gtk_tree_view_column_new_with_attributes ("Value", renderer,
    "text", SWFEDIT_COLUMN_VALUE, "visible", SWFEDIT_COLUMN_VALUE_VISIBLE, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  return TRUE;
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  if (argc <= 1) {
    g_print ("Usage: %s FILENAME\n", argv[0]);
    return 1;
  }
  if (open_window (argv[1])) {
    gtk_main ();
    return 0;
  } else {
    return 1;
  }
}

