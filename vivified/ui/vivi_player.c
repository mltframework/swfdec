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
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (vivi_vivi_docklet_find_widget_by_type (docklet, SWFDEC_TYPE_GTK_WIDGET));

  g_signal_connect (app, "notify", G_CALLBACK (vivi_player_notify_app), widget);
  swfdec_gtk_widget_set_player (widget, vivi_application_get_player (app));
  swfdec_gtk_widget_set_interactive (widget, !vivi_application_get_interrupted (app));
}

void
vivi_player_application_unset (ViviViviDocklet *docklet, ViviApplication *app);
void
vivi_player_application_unset (ViviViviDocklet *docklet, ViviApplication *app)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (vivi_vivi_docklet_find_widget_by_type (docklet, SWFDEC_TYPE_GTK_WIDGET));

  g_signal_handlers_disconnect_by_func (app, vivi_player_notify_app, widget);
}

