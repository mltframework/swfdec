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

#include "vivi_vivi_docklet.h"

enum {
  PROP_0,
  PROP_APP
};

enum {
  APPLICATION_SET,
  APPLICATION_UNSET,
  LAST_SIGNAL
};

G_DEFINE_TYPE (ViviViviDocklet, vivi_vivi_docklet, VIVI_TYPE_DOCKLET)
guint signals[LAST_SIGNAL];

static void
vivi_vivi_docklet_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  ViviViviDocklet *docklet = VIVI_VIVI_DOCKLET (object);
  
  switch (param_id) {
    case PROP_APP:
      g_value_set_object (value, docklet->app);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_vivi_docklet_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  ViviViviDocklet *docklet = VIVI_VIVI_DOCKLET (object);

  switch (param_id) {
    case PROP_APP:
      if (docklet->app) {
	g_signal_emit (docklet, signals[APPLICATION_UNSET], 0, docklet->app);
	g_object_unref (docklet->app);
      }
      docklet->app = g_value_dup_object (value);
      if (docklet->app) {
	g_signal_emit (docklet, signals[APPLICATION_SET], 0, docklet->app);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_vivi_docklet_dispose (GObject *object)
{
  ViviViviDocklet *docklet = VIVI_VIVI_DOCKLET (object);

  if (docklet->app) {
    g_signal_emit (docklet, signals[APPLICATION_UNSET], 0, docklet->app);
    g_object_unref (docklet->app);
    docklet->app = NULL;
  }

  G_OBJECT_CLASS (vivi_vivi_docklet_parent_class)->dispose (object);
}

static void
vivi_vivi_docklet_class_init (ViviViviDockletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_vivi_docklet_dispose;
  object_class->get_property = vivi_vivi_docklet_get_property;
  object_class->set_property = vivi_vivi_docklet_set_property;

  g_object_class_install_property (object_class, PROP_APP,
      g_param_spec_object ("application", "application", "application used by this docklet",
	  VIVI_TYPE_APPLICATION, G_PARAM_READWRITE));

  signals[APPLICATION_SET] = g_signal_new ("application-set", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ViviViviDockletClass, application_set), 
      NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, VIVI_TYPE_APPLICATION);
  signals[APPLICATION_UNSET] = g_signal_new ("application-unset", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (ViviViviDockletClass, application_unset), 
      NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
      G_TYPE_NONE, 1, VIVI_TYPE_APPLICATION);
}

static void
vivi_vivi_docklet_init (ViviViviDocklet *docklet)
{
}

typedef struct {
  GtkWidget *	result;
  GType		desired_type;
} FindData;

static void
find_widget (GtkWidget *widget, gpointer datap)
{
  FindData *data = datap;

  if (G_TYPE_CHECK_INSTANCE_TYPE (widget, data->desired_type)) {
    data->result = widget;
    return;
  }
  if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), find_widget, data);
}

GtkWidget *
vivi_vivi_docklet_find_widget_by_type (ViviViviDocklet *docklet, GType type)
{
  FindData data = { NULL, };

  g_return_val_if_fail (VIVI_IS_VIVI_DOCKLET (docklet), NULL);
  g_return_val_if_fail (g_type_is_a (type, GTK_TYPE_WIDGET), NULL);

  data.desired_type = type;
  gtk_container_foreach (GTK_CONTAINER (docklet), find_widget, &data);
  return data.result;
}
