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
#include <swfdec-gtk/swfdec-gtk.h>
#include "vivified/core/vivified-core.h"
#include "vivified/dock/vivified-dock.h"
#include "vivi_window.h"

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
setup (const char *filename, const char *variables)
{
  GtkWidget *window;
  ViviApplication *app;

  app = vivi_application_new ();
  vivi_application_set_filename (app, filename);
  vivi_application_set_variables (app, variables);

  window = vivi_window_new (app);

  g_signal_connect_swapped (app, "notify::quit", G_CALLBACK (gtk_widget_destroy), window);
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
