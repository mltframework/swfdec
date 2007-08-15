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
#include "vivi_commandline.h"

G_DEFINE_TYPE (ViviCommandLine, vivi_command_line, VIVI_TYPE_DOCKLET)

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

static void
vivi_command_line_dispose (GObject *object)
{
  ViviCommandLine *cl = VIVI_COMMAND_LINE (object);

  g_signal_handlers_disconnect_by_func (cl->app, vivi_command_line_append_message, cl->view);

  G_OBJECT_CLASS (vivi_command_line_parent_class)->dispose (object);
}

static void
vivi_command_line_class_init (ViviCommandLineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_command_line_dispose;
}

static void
vivi_command_line_execute (ViviCommandLine *cl, const char *command)
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

  
  vivi_application_execute (cl->app, run);
  if (command != run)
    g_free (run);
}

static void
command_line_entry_activate_cb (GtkEntry *entry, ViviCommandLine *command_line)
{
  const char *text = gtk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  vivi_command_line_execute (command_line, text);
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
}

static void
vivi_command_line_init (ViviCommandLine *cl)
{
  GtkWidget *box, *widget, *scroll;

  box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (cl), box);
  /* the text entry */
  widget = gtk_entry_new ();
  g_signal_connect (widget, "activate", G_CALLBACK (command_line_entry_activate_cb), cl);
  gtk_box_pack_end (GTK_BOX (box), widget, FALSE, TRUE, 0);
  /* the text view for outputting messages */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);
  cl->view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (cl->view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (cl->view), GTK_WRAP_WORD_CHAR);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (cl->view)),
      "error", "foreground", "red", "left-margin", 15, NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (cl->view)),
      "input", "foreground", "dark grey", NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (cl->view)),
      "output", "left-margin", 15, NULL);
  gtk_container_add (GTK_CONTAINER (scroll), cl->view);

  gtk_widget_show_all (box);
}

GtkWidget *
vivi_command_line_new (ViviApplication *app)
{
  ViviCommandLine *cl;

  cl = g_object_new (VIVI_TYPE_COMMAND_LINE, "title", "Command Line", NULL);
  cl->app = app;
  g_signal_connect (cl->app, "message", G_CALLBACK (vivi_command_line_append_message), cl->view);
  return GTK_WIDGET (cl);
}

