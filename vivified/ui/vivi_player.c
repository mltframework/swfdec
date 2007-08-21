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

#include <libswfdec-gtk/swfdec-gtk.h>
#include "vivi_vivi_docklet.h"

static void
find_player (GtkWidget *widget, gpointer result)
{
  if (SWFDEC_IS_GTK_WIDGET (widget)) {
    *(gpointer *) result = widget;
    return;
  }

  if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), find_player, result);
}

static void
vivi_player_notify_app (ViviApplication *app, GParamSpec *pspec, SwfdecGtkWidget *player)
{
  if (g_str_equal (pspec->name, "player")) {
    swfdec_gtk_widget_set_player (player, vivi_application_get_player (app));
  } else if (g_str_equal (pspec->name, "interrupted")) {
    swfdec_gtk_widget_set_interactive (player, !vivi_application_get_interrupted (app));
  }
}

void
vivi_player_application_set (ViviViviDocklet *docklet, ViviApplication *app);
void
vivi_player_application_set (ViviViviDocklet *docklet, ViviApplication *app)
{
  SwfdecGtkPlayer *player = NULL;

  find_player (GTK_WIDGET (docklet), &player);

  g_signal_connect (app, "notify", G_CALLBACK (vivi_player_notify_app), player);
  swfdec_gtk_widget_set_player (SWFDEC_GTK_WIDGET (player), vivi_application_get_player (app));
  swfdec_gtk_widget_set_interactive (SWFDEC_GTK_WIDGET (player), !vivi_application_get_interrupted (app));
}

void
vivi_player_application_unset (ViviViviDocklet *docklet, ViviApplication *app);
void
vivi_player_application_unset (ViviViviDocklet *docklet, ViviApplication *app)
{
  SwfdecGtkPlayer *player = NULL;

  find_player (GTK_WIDGET (docklet), &player);

  g_signal_handlers_disconnect_by_func (app, vivi_player_notify_app, player);
}

