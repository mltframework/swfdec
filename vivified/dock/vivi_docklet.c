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

#include "vivi_docklet.h"

enum {
  PROP_0,
  PROP_TITLE,
  PROP_ICON
};

G_DEFINE_ABSTRACT_TYPE (ViviDocklet, vivi_docklet, GTK_TYPE_BIN)

static void
vivi_docklet_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  ViviDocklet *docklet = VIVI_DOCKLET (object);
  
  switch (param_id) {
    case PROP_TITLE:
      g_value_set_string (value, docklet->title);
      break;
    case PROP_ICON:
      g_value_set_string (value, docklet->icon);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_docklet_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  ViviDocklet *docklet = VIVI_DOCKLET (object);

  switch (param_id) {
    case PROP_TITLE:
      vivi_docklet_set_title (docklet, g_value_get_string (value));
      break;
    case PROP_ICON:
      vivi_docklet_set_title (docklet, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_docklet_dispose (GObject *object)
{
  ViviDocklet *docklet = VIVI_DOCKLET (object);

  g_free (docklet->title);
  g_free (docklet->icon);

  G_OBJECT_CLASS (vivi_docklet_parent_class)->dispose (object);
}

static void
vivi_docklet_size_request (GtkWidget *widget, GtkRequisition *req)
{
  GtkWidget *child = GTK_BIN (widget)->child;
  
  if (child) {
    gtk_widget_size_request (child, req);
  } else {
    req->width = req->height = 0;
  }
}

static void
vivi_docklet_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GtkWidget *child = GTK_BIN (widget)->child;
  
  GTK_WIDGET_CLASS (vivi_docklet_parent_class)->size_allocate (widget, allocation);

  if (child && GTK_WIDGET_VISIBLE (child)) {
    gtk_widget_size_allocate (child, allocation);
  }
}

static void
vivi_docklet_class_init (ViviDockletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = vivi_docklet_dispose;
  object_class->get_property = vivi_docklet_get_property;
  object_class->set_property = vivi_docklet_set_property;

  g_object_class_install_property (object_class, PROP_TITLE,
      g_param_spec_string ("title", "title", "title of this docklet",
	  "Unnamed", G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ICON,
      g_param_spec_string ("icon", "icon", "name of the icon to display",
	  GTK_STOCK_MISSING_IMAGE, G_PARAM_READWRITE));

  widget_class->size_request = vivi_docklet_size_request;
  widget_class->size_allocate = vivi_docklet_size_allocate;
}

static void
vivi_docklet_init (ViviDocklet *docklet)
{
  docklet->title = g_strdup ("Unnamed");
  docklet->icon = g_strdup (GTK_STOCK_MISSING_IMAGE);
}

void
vivi_docklet_set_title (ViviDocklet *docklet, const char *title)
{
  g_return_if_fail (VIVI_IS_DOCKLET (docklet));
  g_return_if_fail (title != NULL);

  g_free (docklet->title);
  docklet->title = g_strdup (title);
  g_object_notify (G_OBJECT (docklet), "title");
}

const char *
vivi_docklet_get_title (ViviDocklet *docklet)
{
  g_return_val_if_fail (VIVI_IS_DOCKLET (docklet), NULL);

  return docklet->title;
}

void
vivi_docklet_set_icon (ViviDocklet *docklet, const char *icon)
{
  g_return_if_fail (VIVI_IS_DOCKLET (docklet));
  g_return_if_fail (icon != NULL);

  g_free (docklet->icon);
  docklet->icon = g_strdup (icon);
  g_object_notify (G_OBJECT (docklet), "icon");
}

const char *
vivi_docklet_get_icon (ViviDocklet *docklet)
{
  g_return_val_if_fail (VIVI_IS_DOCKLET (docklet), NULL);

  return docklet->icon;
}

