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
setup (const char *filename)
{
  GtkWidget *window, *box, *widget;
  ViviApplication *app;

  app = vivi_application_new ();
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  box = vivi_vdock_new ();
  gtk_container_add (GTK_CONTAINER (window), box);
  widget = vivi_player_new (app);
  gtk_container_add (GTK_CONTAINER (box), widget);
  widget = vivi_command_line_new (app);
  gtk_container_add (GTK_CONTAINER (box), widget);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (window);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  if (argc != 2) {
    g_print ("usage: %s FILE\n", argv[0]);
    return 0;
  }

  setup (argv[1]);
  gtk_main ();

  return 0;
}
