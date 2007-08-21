/* Vivified
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

#include <string.h>
#include "vivi_vivi_docklet.h"

static void
vivi_command_line_execute (ViviApplication *app, const char *command)
{
  char *run;

  if (!strpbrk (command, ";\"',()[]{}")) {
    /* special mode: interpret as space-delimited list:
     * first argument is function name, following arguemnts are function arguments
     */
    char **args = g_strsplit (command, " ", -1);
    GString *str = g_string_new (args[0]);
    guint i;

    g_string_append (str, " (");
    for (i = 1; args[i] != NULL; i++) {
      if (i > 1)
	g_string_append (str, ", ");
      g_string_append_c (str, '"');
      g_string_append (str, args[i]);
      g_string_append_c (str, '"');
    }
    g_string_append (str, ");");
    run = g_string_free (str, FALSE);
  } else {
    run = (char *) command;
  }

  
  vivi_application_execute (app, run);
  if (command != run)
    g_free (run);
}

void
vivi_command_line_activate (GtkEntry *entry, ViviApplication *app);
void
vivi_command_line_activate (GtkEntry *entry, ViviApplication *app)
{
  const char *text = gtk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  vivi_command_line_execute (app, text);
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
}

static void
vivi_command_line_append_message (ViviApplication *app, guint type, const char *message, GtkTextView *view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
  GtkTextIter iter;
  GtkTextMark *mark;
  const char *tag_names[] = { "input", "output", "error" };

  gtk_text_buffer_get_end_iter (buffer, &iter);
  mark = gtk_text_buffer_get_mark (buffer, "end");
  if (mark == NULL)
    mark = gtk_text_buffer_create_mark (buffer, "end", &iter, FALSE);
  if (gtk_text_buffer_get_char_count (buffer) > 0)
    gtk_text_buffer_insert (buffer, &iter, "\n", 1);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, message, -1, tag_names[type], NULL);
  gtk_text_view_scroll_to_mark (view, mark, 0.0, TRUE, 0.0, 0.0);
}

void
vivi_command_line_application_unset (ViviViviDocklet *docklet, ViviApplication *app);
void
vivi_command_line_application_unset (ViviViviDocklet *docklet, ViviApplication *app)
{
  GtkWidget *view = vivi_vivi_docklet_find_widget_by_type (docklet, GTK_TYPE_TEXT_VIEW);

  g_signal_handlers_disconnect_by_func (app, vivi_command_line_append_message, view);
}

void
vivi_command_line_application_set (ViviViviDocklet *docklet, ViviApplication *app);
void
vivi_command_line_application_set (ViviViviDocklet *docklet, ViviApplication *app)
{
  GtkWidget *view = vivi_vivi_docklet_find_widget_by_type (docklet, GTK_TYPE_TEXT_VIEW);

  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
      "error", "foreground", "red", "left-margin", 15, NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
      "input", "foreground", "dark grey", NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)),
      "output", "left-margin", 15, NULL);

  g_signal_connect (app, "message", G_CALLBACK (vivi_command_line_append_message), view);
}

