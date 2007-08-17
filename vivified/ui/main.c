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

#include <gtk/gtk.h>
#include <libswfdec-gtk/swfdec-gtk.h>
#include "vivified/core/vivified-core.h"
#include "vivified/dock/vivified-dock.h"
#include "vivi_commandline.h"
#include "vivi_player.h"

static void
try_grab_focus (GtkWidget *widget, gpointer unused)
{
  if (GTK_IS_ENTRY (widget))
    gtk_widget_grab_focus (widget);
  else if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), try_grab_focus, NULL);
}

static gboolean
delete_event (GtkWidget *widget, GdkEvent *event, ViviApplication *app)
{
  if (!vivi_application_is_quit (app)) {
    vivi_application_quit (app);
    return TRUE;
  }
  return FALSE;
}

static void
set_title (ViviApplication *app, GParamSpec *pspec, GtkWindow *window)
{
  const char *filename = vivi_application_get_filename (app);

  if (filename == NULL)
    filename = "Vivified";
  gtk_window_set_title (window, filename);
}

static void
setup (const char *filename, const char *variables)
{
  GtkWidget *window, *box, *widget;
  ViviApplication *app;

  app = vivi_application_new ();
  vivi_application_set_filename (app, filename);
  vivi_application_set_variables (app, variables);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 450);
  g_signal_connect_swapped (app, "notify::quit", G_CALLBACK (gtk_widget_destroy), window);
  g_signal_connect (app, "notify::filename", G_CALLBACK (set_title), window);
  set_title (app, NULL, GTK_WINDOW (window));
  box = vivi_vdock_new ();
  gtk_container_add (GTK_CONTAINER (window), box);
  widget = vivi_player_new (app);
  vivi_vdock_add (VIVI_VDOCK (box), widget);
  widget = vivi_command_line_new (app);
  vivi_vdock_add (VIVI_VDOCK (box), widget);
  gtk_container_foreach (GTK_CONTAINER (widget), try_grab_focus, NULL);

  g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), app);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), app);
  gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  if (argc < 2) {
    g_print ("usage: %s FILE [VARIABLES]\n", argv[0]);
    return 0;
  }

  setup (argv[1], argc > 2 ? argv[2] : NULL);
  gtk_main ();

  return 0;
}
