/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_player_manager.h"
#include "swfdec_source.h"

enum {
  PROP_0,
  PROP_PLAYING,
  PROP_SPEED
};

G_DEFINE_TYPE (SwfdecPlayerManager, swfdec_player_manager, G_TYPE_OBJECT)

static void
swfdec_player_manager_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);
  
  switch (param_id) {
    case PROP_PLAYING:
      g_value_set_boolean (value, swfdec_player_manager_get_playing (manager));
      break;
    case PROP_SPEED:
      g_value_set_double (value, swfdec_player_manager_get_speed (manager));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_manager_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);

  switch (param_id) {
    case PROP_PLAYING:
      swfdec_player_manager_set_playing (manager, g_value_get_boolean (value));
      break;
    case PROP_SPEED:
      swfdec_player_manager_set_speed (manager, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_manager_dispose (GObject *object)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);

  swfdec_player_manager_set_playing (manager, FALSE);
  g_object_unref (manager->player);

  G_OBJECT_CLASS (swfdec_player_manager_parent_class)->dispose (object);
}

static void
swfdec_player_manager_class_init (SwfdecPlayerManagerClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_player_manager_dispose;
  object_class->set_property = swfdec_player_manager_set_property;
  object_class->get_property = swfdec_player_manager_get_property;

  g_object_class_install_property (object_class, PROP_SPEED,
      g_param_spec_double ("speed", "speed", "playback speed of movie",
	  G_MINDOUBLE, 16.0, 1.0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SPEED,
      g_param_spec_boolean ("playing", "playing", "if the movie is played back",
	  FALSE, G_PARAM_READWRITE));
}

static void
swfdec_player_manager_init (SwfdecPlayerManager *manager)
{
  manager->speed = 1.0;
}

static void
swfdec_player_manager_set_player (SwfdecPlayerManager *manager, SwfdecPlayer *player)
{
  if (manager->player == player)
    return;

  if (manager->player) {
    g_object_unref (manager->player);
  }
  manager->player = player;
  if (player) {
    g_object_ref (player);
  }
}

SwfdecPlayerManager *
swfdec_player_manager_new (SwfdecPlayer *player)
{
  SwfdecPlayerManager *manager;
  
  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);

  manager = g_object_new (SWFDEC_TYPE_PLAYER_MANAGER, 0);
  swfdec_player_manager_set_player (manager, player);

  return manager;
}

void
swfdec_player_manager_set_speed (SwfdecPlayerManager *manager, double speed)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (speed > 0.0);

  if (manager->speed == speed)
    return;
  if (manager->source) {
    swfdec_player_manager_set_playing (manager, FALSE);
    manager->speed = speed;
    swfdec_player_manager_set_playing (manager, TRUE);
  } else {
    manager->speed = speed;
  }
  g_object_notify (G_OBJECT (manager), "speed");
}

double		
swfdec_player_manager_get_speed (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), 1.0);

  return manager->speed;
}

void
swfdec_player_manager_set_playing (SwfdecPlayerManager *manager, gboolean playing)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));

  if ((manager->source != NULL) == playing)
    return;
  if (playing) {
    g_assert (manager->source == NULL);
    manager->source = swfdec_iterate_source_new (manager->player);
    g_source_attach (manager->source, NULL);
  } else {
    g_assert (manager->source != NULL);
    g_source_destroy (manager->source);
    g_source_unref (manager->source);
    manager->source = NULL;
  }
  g_object_notify (G_OBJECT (manager), "playing");
}

gboolean
swfdec_player_manager_get_playing (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), FALSE);

  return manager->source != NULL;
}

