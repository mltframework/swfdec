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

#include "vivi_commandline.h"

G_DEFINE_TYPE (ViviCommandLine, vivi_command_line, VIVI_TYPE_DOCKLET)

static void
vivi_command_line_dispose (GObject *object)
{
  //ViviCommandLine *command_line = VIVI_COMMAND_LINE (object);

  G_OBJECT_CLASS (vivi_command_line_parent_class)->dispose (object);
}

static void
vivi_command_line_class_init (ViviCommandLineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_command_line_dispose;
}

static void
command_line_entry_activate_cb (GtkEntry *entry, ViviCommandLine *command_line)
{
  const char *text = gtk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  g_print ("%s\n", text);
  //swfdec_player_manager_execute (manager, text);
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
}

static void
vivi_command_line_init (ViviCommandLine *command_line)
{
  GtkWidget *entry;

  entry = gtk_entry_new ();
  gtk_container_add (GTK_CONTAINER (command_line), entry);
  g_signal_connect (entry, "activate", G_CALLBACK (command_line_entry_activate_cb), command_line);
}

GtkWidget *
vivi_command_line_new (ViviApplication *app)
{
  GtkWidget *cl;

  cl = g_object_new (VIVI_TYPE_COMMAND_LINE, "title", "Command Line", NULL);
  return cl;
}

