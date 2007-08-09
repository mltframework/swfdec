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

#include "vivi_docker.h"

enum {
  REQUEST_CLOSE,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (ViviDocker, vivi_docker, GTK_TYPE_EXPANDER)

static void
vivi_docker_dispose (GObject *object)
{
  //ViviDocker *docker = VIVI_DOCKER (object);

  G_OBJECT_CLASS (vivi_docker_parent_class)->dispose (object);
}

static void
vivi_docker_class_init (ViviDockerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_docker_dispose;

  signals[REQUEST_CLOSE] = 0;
}

static void
vivi_docker_init (ViviDocker *docker)
{
}

static void
vivi_docker_docklet_notify_title (ViviDocklet *docklet, GParamSpec *pspec, GtkLabel *label)
{
  gtk_label_set_text (label, vivi_docklet_get_title (docklet));
}

static void
vivi_docker_set_docklet (ViviDocker *docker, ViviDocklet *docklet)
{
  GtkWidget *box, *widget;

  g_return_if_fail (VIVI_IS_DOCKER (docker));
  g_return_if_fail (VIVI_IS_DOCKLET (docklet));

  box = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (box);
  widget = gtk_label_new (vivi_docklet_get_title (docklet));
  gtk_widget_show (widget);
  g_signal_connect (docklet, "notify::title", G_CALLBACK (vivi_docker_docklet_notify_title), widget);
  gtk_box_pack_start (GTK_BOX (box), widget, TRUE, TRUE, 0);
  gtk_expander_set_label_widget (GTK_EXPANDER (docker), box);
  gtk_container_add (GTK_CONTAINER (docker), GTK_WIDGET (docklet));
}

GtkWidget *
vivi_docker_new (ViviDocklet *docklet)
{
  GtkWidget *widget;

  g_return_val_if_fail (VIVI_IS_DOCKLET (docklet), NULL);

  widget = g_object_new (VIVI_TYPE_DOCKER, "expanded", TRUE, NULL);
  vivi_docker_set_docklet (VIVI_DOCKER (widget), docklet);
  return widget;
}


