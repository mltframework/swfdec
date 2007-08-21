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

G_DEFINE_ABSTRACT_TYPE (ViviViviDocklet, vivi_vivi_docklet, VIVI_TYPE_DOCKLET)

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
  ViviViviDockletClass *klass;

  switch (param_id) {
    case PROP_APP:
      docklet->app = g_value_dup_object (value);
      klass = VIVI_VIVI_DOCKLET_GET_CLASS (docklet);
      if (klass->set_app)
	klass->set_app (docklet, docklet->app);
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
  ViviViviDockletClass *klass;

  klass = VIVI_VIVI_DOCKLET_GET_CLASS (docklet);
  if (klass->unset_app)
    klass->unset_app (docklet, docklet->app);
  g_object_unref (docklet->app);

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
	  VIVI_TYPE_APPLICATION, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
vivi_vivi_docklet_init (ViviViviDocklet *vivi_docklet)
{
}

