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

static void
entry_activate_cb (GtkEntry *entry, ViviApplication *app)
{
  const char *text = gtk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  //swfdec_player_manager_execute (manager, text);
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
}

static void
setup (const char *filename)
{
  GtkWidget *window, *box, *widget;
  ViviApplication *app;

  app = vivi_application_new ();
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);
  /* widget displaying the Flash */
  widget = swfdec_gtk_widget_new (NULL);
  gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
  /* text entry */
  widget = gtk_entry_new ();
  g_signal_connect (widget, "activate", G_CALLBACK (entry_activate_cb), app);
  gtk_box_pack_end (GTK_BOX (box), widget, FALSE, TRUE, 0);

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
