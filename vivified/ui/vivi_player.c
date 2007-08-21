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

#include "vivi_player.h"
#include <libswfdec-gtk/swfdec-gtk.h>

G_DEFINE_TYPE (ViviPlayer, vivi_player, VIVI_TYPE_VIVI_DOCKLET)

static void
vivi_player_notify_app (ViviApplication *app, GParamSpec *pspec, ViviPlayer *player)
{
  if (g_str_equal (pspec->name, "player")) {
    swfdec_gtk_widget_set_player (SWFDEC_GTK_WIDGET (player->player), vivi_application_get_player (app));
  } else if (g_str_equal (pspec->name, "interrupted")) {
    swfdec_gtk_widget_set_interactive (SWFDEC_GTK_WIDGET (player->player), 
	!vivi_application_get_interrupted (app));
  }
}

static void
vivi_player_application_set (ViviViviDocklet *docklet, ViviApplication *app)
{
  ViviPlayer *player = VIVI_PLAYER (docklet);

  g_signal_connect (app, "notify", G_CALLBACK (vivi_player_notify_app), player);
  swfdec_gtk_widget_set_player (SWFDEC_GTK_WIDGET (player->player), vivi_application_get_player (app));
}

static void
vivi_player_application_unset (ViviViviDocklet *docklet, ViviApplication *app)
{
  g_signal_handlers_disconnect_by_func (app, vivi_player_notify_app, docklet);
}

static void
vivi_player_class_init (ViviPlayerClass *klass)
{
  ViviViviDockletClass *vivi_docklet_class = VIVI_VIVI_DOCKLET_CLASS (klass);

  vivi_docklet_class->application_set = vivi_player_application_set;
  vivi_docklet_class->application_unset = vivi_player_application_unset;
}

static void
vivi_player_init (ViviPlayer *player)
{
  GtkWidget *box;

  box = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (player), box);
  /* the player */
  player->player = swfdec_gtk_widget_new (NULL);
  gtk_container_add (GTK_CONTAINER (box), player->player);

  gtk_widget_show_all (box);
}

GtkWidget *
vivi_player_new (ViviApplication *app)
{
  return g_object_new (VIVI_TYPE_PLAYER, "title", "Player", "application", app, NULL);
}

