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

#include "vivi_vdock.h"
#include "vivi_docker.h"
#include "vivi_docklet.h"

G_DEFINE_TYPE (ViviVDock, vivi_vdock, GTK_TYPE_BIN)

static void
vivi_vdock_destroy (GtkObject *object)
{
  ViviVDock *vdock = VIVI_VDOCK (object);

  GTK_OBJECT_CLASS (vivi_vdock_parent_class)->destroy (object);

  g_list_free (vdock->docklets);
  vdock->docklets = NULL;
}

static void
vivi_vdock_size_request (GtkWidget *widget, GtkRequisition *req)
{
  GtkWidget *child = GTK_BIN (widget)->child;
  
  if (child) {
    gtk_widget_size_request (child, req);
  } else {
    req->width = req->height = 0;
  }
}

static void
vivi_vdock_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GtkWidget *child = GTK_BIN (widget)->child;
  
  GTK_WIDGET_CLASS (vivi_vdock_parent_class)->size_allocate (widget, allocation);

  if (child && GTK_WIDGET_VISIBLE (child)) {
    gtk_widget_size_allocate (child, allocation);
  }
}

void
vivi_vdock_add (ViviVDock *vdock, GtkWidget *widget)
{
  GtkWidget *docker;
  
  g_return_if_fail (VIVI_IS_VDOCK (vdock));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  docker = vivi_docker_new (VIVI_DOCKLET (widget));
  gtk_widget_show (docker);

  g_object_ref (widget);
  if (vdock->docklets == NULL) {
    gtk_container_add (GTK_CONTAINER (vdock), docker);
  } else {
    /* docklet is in docker, so we need to use parent */
    GtkWidget *last = gtk_widget_get_parent (vdock->docklets->data);
    GtkWidget *parent = gtk_widget_get_parent (last);
    GtkWidget *paned;

    g_object_ref (last);
    gtk_container_remove (GTK_CONTAINER (parent), last);
    paned = gtk_vpaned_new ();
    gtk_paned_pack1 (GTK_PANED (paned), last, TRUE, FALSE);
    gtk_paned_pack2 (GTK_PANED (paned), docker, TRUE, FALSE);
    g_object_unref (last);
    gtk_widget_show (paned);
    if (parent == (GtkWidget *) vdock) {
      gtk_container_add (GTK_CONTAINER (vdock), paned);
    } else {
      gtk_paned_pack2 (GTK_PANED (parent), paned, TRUE, FALSE);
    }
  }
  vdock->docklets = g_list_prepend (vdock->docklets, widget);
}

void
vivi_vdock_remove (ViviVDock *vdock, GtkWidget *widget)
{
  GtkWidget *docker, *parent;

  g_return_if_fail (g_list_find (vdock->docklets, widget));

  docker = gtk_widget_get_parent (widget);
  parent = gtk_widget_get_parent (docker);
  if (parent == (GtkWidget *) vdock) {
    gtk_container_remove (GTK_CONTAINER (vdock), docker);
  } else {
    GtkWidget *other;
    GtkWidget *paned_parent;
    g_assert (GTK_IS_PANED (parent));
    paned_parent = gtk_widget_get_parent (parent);
    other = gtk_paned_get_child1 (GTK_PANED (parent));
    if (other == docker)
      other = gtk_paned_get_child2 (GTK_PANED (parent));
    g_object_ref (other);
    gtk_container_remove (GTK_CONTAINER (parent), docker);
    gtk_container_remove (GTK_CONTAINER (parent), other);
    if (paned_parent == (GtkWidget *) vdock) {
      gtk_container_remove (GTK_CONTAINER (vdock), parent);
      gtk_container_add (GTK_CONTAINER (vdock), other);
    } else {
      gtk_container_remove (GTK_CONTAINER (paned_parent), parent);
      gtk_paned_pack1 (GTK_PANED (parent), other, TRUE, FALSE);
    }
    g_object_unref (other);
  }
  vdock->docklets = g_list_remove (vdock->docklets, widget);
  g_object_unref (widget);
}

static void
vivi_vdock_class_init (ViviVDockClass *klass)
{
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->destroy = vivi_vdock_destroy;

  widget_class->size_request = vivi_vdock_size_request;
  widget_class->size_allocate = vivi_vdock_size_allocate;
}

static void
vivi_vdock_init (ViviVDock *vdock)
{
}

GtkWidget *
vivi_vdock_new (void)
{
  return g_object_new (VIVI_TYPE_VDOCK, NULL);
}


