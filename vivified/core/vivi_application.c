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

#include "vivi_application.h"
#include <libswfdec-gtk/swfdec-gtk.h>

enum {
  PROP_0,
  PROP_FILENAME,
  PROP_PLAYER
};

G_DEFINE_TYPE (ViviApplication, vivi_application, SWFDEC_TYPE_AS_CONTEXT)

static void
vivi_application_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  ViviApplication *app = VIVI_APPLICATION (object);
  
  switch (param_id) {
    case PROP_FILENAME:
      g_value_set_string (value, app->filename);
      break;
    case PROP_PLAYER:
      g_value_set_object (value, app->player);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_application_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  ViviApplication *app = VIVI_APPLICATION (object);

  switch (param_id) {
    case PROP_FILENAME:
      vivi_application_set_filename (app, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_application_dispose (GObject *object)
{
  ViviApplication *app = VIVI_APPLICATION (object);

  g_object_unref (app->player);

  G_OBJECT_CLASS (vivi_application_parent_class)->dispose (object);
}

static void
vivi_application_class_init (ViviApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_application_dispose;
  object_class->get_property = vivi_application_get_property;
  object_class->set_property = vivi_application_set_property;

  g_object_class_install_property (object_class, PROP_FILENAME,
      g_param_spec_string ("filename", "filename", "name of file to play",
	  NULL, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLAYER,
      g_param_spec_object ("player", "player", "Flash player in use",
	  SWFDEC_TYPE_PLAYER, G_PARAM_READABLE));
}

static void
vivi_application_init (ViviApplication *app)
{
  app->player = swfdec_gtk_player_new ();
}

ViviApplication *
vivi_application_new (void)
{
  return g_object_new (VIVI_TYPE_APPLICATION, NULL);
}

void
vivi_application_reset (ViviApplication *app)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));

  g_object_unref (app->player);
  app->player = swfdec_player_new ();
}

void
vivi_application_set_filename (ViviApplication *app, const char *filename)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));
  g_return_if_fail (filename != NULL);

  g_free (app->filename);
  app->filename = g_strdup (filename);
  vivi_application_reset (app);
  g_object_notify (G_OBJECT (app), "filename");
}

const char *
vivi_application_get_filename (ViviApplication *app)
{
  g_return_val_if_fail (VIVI_IS_APPLICATION (app), NULL);

  return app->filename;
}

SwfdecPlayer *
vivi_application_get_player (ViviApplication *app)
{
  g_return_val_if_fail (VIVI_IS_APPLICATION (app), NULL);

  return app->player;
}

