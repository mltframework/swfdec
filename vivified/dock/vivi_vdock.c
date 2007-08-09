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
vivi_vdock_dispose (GObject *object)
{
  ViviVDock *vdock = VIVI_VDOCK (object);

  g_list_free (vdock->docklets);
  vdock->docklets = NULL;

  G_OBJECT_CLASS (vivi_vdock_parent_class)->dispose (object);
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

static void
vivi_vdock_add (GtkContainer *container, GtkWidget *widget)
{
  ViviVDock *vdock = VIVI_VDOCK (container);
  GtkWidget *docker;
  
  docker = vivi_docker_new (VIVI_DOCKLET (widget));
  gtk_widget_show (docker);

  g_object_ref (widget);
  if (vdock->docklets == NULL) {
    GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->add (container, docker);
  } else {
    /* docklet is in docker */
    GtkWidget *last = gtk_widget_get_parent (vdock->docklets->data);
    GtkWidget *parent = gtk_widget_get_parent (last);
    GtkWidget *paned;

    g_object_ref (parent);
    if (parent == (GtkWidget *) container) {
      GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->remove (container, last);
    } else {
      gtk_container_remove (GTK_CONTAINER (parent), last);
    }
    paned = gtk_vpaned_new ();
    gtk_paned_pack1 (GTK_PANED (paned), docker, TRUE, FALSE);
    gtk_paned_pack2 (GTK_PANED (paned), last, TRUE, FALSE);
    g_object_unref (last);
    gtk_widget_show (paned);
    if (parent == (GtkWidget *) container) {
      GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->add (container, paned);
    } else {
      gtk_paned_pack1 (GTK_PANED (parent), paned, TRUE, FALSE);
    }
  }
  vdock->docklets = g_list_prepend (vdock->docklets, widget);
}

static void
vivi_vdock_remove (GtkContainer *container, GtkWidget *widget)
{
  ViviVDock *vdock = VIVI_VDOCK (container);
  GtkWidget *docker, *parent;

  g_return_if_fail (g_list_find (vdock->docklets, widget));

  docker = gtk_widget_get_parent (widget);
  parent = gtk_widget_get_parent (docker);
  if (parent == (GtkWidget *) container) {
    GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->remove (container, docker);
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
    if (paned_parent == (GtkWidget *) container) {
      GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->remove (container, parent);
      GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->remove (container, other);
    } else {
      gtk_container_remove (GTK_CONTAINER (paned_parent), parent);
      gtk_paned_pack1 (GTK_PANED (parent), other, TRUE, FALSE);
    }
    g_object_unref (other);
  }
  vdock->docklets = g_list_remove (vdock->docklets, widget);
  g_object_unref (widget);
}

static GType
vivi_vdock_child_type (GtkContainer *container)
{
  return VIVI_TYPE_DOCKLET;
}

static void
vivi_vdock_forall (GtkContainer *container, gboolean include_internals,
    GtkCallback callback, gpointer callback_data)
{
  if (include_internals) {
    GTK_CONTAINER_CLASS (vivi_vdock_parent_class)->forall (container, include_internals, 
	callback, callback_data);
  } else {
    GList *walk;

    for (walk = VIVI_VDOCK (container)->docklets; walk; walk = walk->next) {
      callback (walk->data, callback_data);
    }
  }
}

static void
vivi_vdock_class_init (ViviVDockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose = vivi_vdock_dispose;

  widget_class->size_request = vivi_vdock_size_request;
  widget_class->size_allocate = vivi_vdock_size_allocate;

  container_class->add = vivi_vdock_add;
  container_class->remove = vivi_vdock_remove;
  container_class->child_type = vivi_vdock_child_type;
  container_class->forall = vivi_vdock_forall;
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


