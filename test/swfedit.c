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

static void
save (GtkButton *button, SwfeditFile *file)
{
  GtkWidget *dialog;
  GError *error = NULL;

  dialog = gtk_file_chooser_dialog_new ("Save file...",
      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_OK, 
      NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), file->filename);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
    g_free (file->filename);
    file->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (!swfedit_file_save (file, &error)) {
      g_printerr ("Error saving file: %s\n", error->message);
      g_error_free (error);
    }
  }
  gtk_widget_destroy (dialog);
}

static void
cell_renderer_edited (GtkCellRenderer *renderer, char *path,
    char *new_text, SwfeditFile *file)
{
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (file),
	&iter, path)) {
    g_assert_not_reached ();
  }
  swfedit_token_set (SWFEDIT_TOKEN (file), &iter, new_text);
}

static gboolean
open_window (char *filename)
{
  SwfeditFile *file;
  GtkWidget *window, *scroll, *box, *button, *treeview;
  GError *error = NULL;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  file = swfedit_file_new (filename, &error);
  if (file == NULL) {
    g_printerr ("Error opening file %s: %s\n", filename, error->message);
    g_error_free (error);
    return FALSE;
  }
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), filename);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  box = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), box);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);

  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (file));
  gtk_container_add (GTK_CONTAINER (scroll), treeview);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name", renderer,
    "text", SWFEDIT_COLUMN_NAME, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
  g_signal_connect (renderer, "edited", G_CALLBACK (cell_renderer_edited), file);
  column = gtk_tree_view_column_new_with_attributes ("Value", renderer,
    "text", SWFEDIT_COLUMN_VALUE, "visible", SWFEDIT_COLUMN_VALUE_VISIBLE, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  g_signal_connect (button, "clicked", G_CALLBACK (save), file);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);

  gtk_widget_show_all (window);
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

