/* Swfdec
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

#include "swfdec-gtk/swfdec_gtk_loader.h"
#include "swfdec-gtk/swfdec_gtk_player.h"
#include "swfdec-gtk/swfdec_gtk_socket.h"
#include "swfdec-gtk/swfdec_playback.h"
#include "swfdec-gtk/swfdec_source.h"

struct _SwfdecGtkPlayer
{
  SwfdecPlayer		player;

  GSource *		source;		/* source if playing, NULL otherwise */
  SwfdecPlayback *	playback;	/* audio playback object */
  gboolean		audio_enabled;	/* TRUE if audio should be played */
  double		speed;		/* desired playback speed */
};

struct _SwfdecGtkPlayerClass
{
  SwfdecPlayerClass   	player_class;
};

enum {
  PROP_0,
  PROP_PLAYING,
  PROP_AUDIO,
  PROP_SPEED
};

/*** gtk-doc ***/

/**
 * SECTION:SwfdecGtkPlayer
 * @title: SwfdecGtkPlayer
 * @short_description: an improved #SwfdecPlayer
 *
 * The #SwfdecGtkPlayer adds common functionality to the rather barebones 
 * #SwfdecPlayer class, such as automatic playback and audio handling. Note 
 * that by default, the player will be paused, so you need to call 
 * swfdec_gtk_player_set_playing () on the new player.
 *
 * @see_also: SwfdecPlayer
 */

/**
 * SwfdecGtkPlayer:
 *
 * The structure for the Swfdec Gtk player contains no public fields.
 */

/*** SWFDEC_GTK_PLAYER ***/

G_DEFINE_TYPE (SwfdecGtkPlayer, swfdec_gtk_player, SWFDEC_TYPE_PLAYER)

static void
swfdec_gtk_player_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecGtkPlayer *player = SWFDEC_GTK_PLAYER (object);
  
  switch (param_id) {
    case PROP_PLAYING:
      g_value_set_boolean (value, player->source != NULL);
      break;
    case PROP_AUDIO:
      g_value_set_boolean (value, player->audio_enabled);
      break;
    case PROP_SPEED:
      g_value_set_double (value, player->speed);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_gtk_player_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecGtkPlayer *player = SWFDEC_GTK_PLAYER (object);
  
  switch (param_id) {
    case PROP_PLAYING:
      swfdec_gtk_player_set_playing (player, g_value_get_boolean (value));
      break;
    case PROP_AUDIO:
      swfdec_gtk_player_set_audio_enabled (player, g_value_get_boolean (value));
      break;
    case PROP_SPEED:
      swfdec_gtk_player_set_speed (player, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_gtk_player_dispose (GObject *object)
{
  SwfdecGtkPlayer *player = SWFDEC_GTK_PLAYER (object);

  swfdec_gtk_player_set_playing (player, FALSE);
  g_assert (player->playback == NULL);

  G_OBJECT_CLASS (swfdec_gtk_player_parent_class)->dispose (object);
}

static void
swfdec_gtk_player_class_init (SwfdecGtkPlayerClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_gtk_player_dispose;
  object_class->get_property = swfdec_gtk_player_get_property;
  object_class->set_property = swfdec_gtk_player_set_property;

  g_object_class_install_property (object_class, PROP_PLAYING,
      g_param_spec_boolean ("playing", "playing", "TRUE if the player is playing (d'oh)",
	  FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_AUDIO,
      g_param_spec_boolean ("audio-enabled", "audio enabled", "TRUE if automatic audio handling is enabled",
	  TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SPEED,
      g_param_spec_double ("speed", "speed", "desired playback speed",
	  G_MINDOUBLE, G_MAXDOUBLE, 1.0, G_PARAM_READWRITE));
}

static void
swfdec_gtk_player_init (SwfdecGtkPlayer * player)
{
  player->speed = 1.0;
  player->audio_enabled = TRUE;
}

/*** PUBLIC API ***/

/**
 * swfdec_gtk_player_new:
 * @debugger: %NULL or a #SwfdecAsDebugger to debug this player
 *
 * Creates a new Swfdec Gtk player.
 * This function calls swfdec_init () for you if it wasn't called before.
 *
 * Returns: The new player
 **/
SwfdecPlayer *
swfdec_gtk_player_new (SwfdecAsDebugger *debugger)
{
  SwfdecPlayer *player;

  swfdec_init ();
  player = g_object_new (SWFDEC_TYPE_GTK_PLAYER, 
      "loader-type", SWFDEC_TYPE_GTK_LOADER, "socket-type", SWFDEC_TYPE_GTK_SOCKET,
      "debugger", debugger, NULL);

  return player;
}

static void
swfdec_gtk_player_update_audio (SwfdecGtkPlayer *player)
{
  gboolean should_play = player->audio_enabled;
  
  should_play &= (player->source != NULL);
  should_play &= (player->speed == 1.0);

  if (should_play && player->playback == NULL) {
    player->playback = swfdec_playback_open (SWFDEC_PLAYER (player),
	g_main_context_default ());
  } else if (!should_play && player->playback != NULL) {
    swfdec_playback_close (player->playback);
    player->playback = NULL;
  }
}

/**
 * swfdec_gtk_player_set_playing:
 * @player: a #SwfdecGtkPlayer
 * @playing: if the player should play or not
 *
 * Sets if @player should be playing or not. If the player is playing, a timer 
 * will be attached to the default main loop that periodically calls 
 * swfdec_player_advance().
 **/
void
swfdec_gtk_player_set_playing (SwfdecGtkPlayer *player, gboolean playing)
{
  g_return_if_fail (SWFDEC_IS_GTK_PLAYER (player));

  if (playing && player->source == NULL) {
    player->source = swfdec_iterate_source_new (SWFDEC_PLAYER (player), player->speed);
    g_source_attach (player->source, NULL);
  } else if (!playing && player->source != NULL) {
    g_source_destroy (player->source);
    g_source_unref (player->source);
    player->source = NULL;
  }
  swfdec_gtk_player_update_audio (player);
  g_object_notify (G_OBJECT (player), "playing");
}

/**
 * swfdec_gtk_player_get_playing:
 * @player: a #SwfdecGtkPlayer
 *
 * Queries if the player is playing.
 *
 * Returns: %TRUE if the player is playing
 **/
gboolean
swfdec_gtk_player_get_playing (SwfdecGtkPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_PLAYER (player), FALSE);

  return player->source != NULL;
}

/**
 * swfdec_gtk_player_set_audio_enabled:
 * @player: a #SwfdecGtkPlayer
 * @enabled: %TRUE to enable audio
 *
 * Enables or disables automatic audio playback.
 **/
void
swfdec_gtk_player_set_audio_enabled (SwfdecGtkPlayer *player, gboolean enabled)
{
  g_return_if_fail (SWFDEC_IS_GTK_PLAYER (player));

  if (player->audio_enabled == enabled)
    return;
  player->audio_enabled = enabled;
  swfdec_gtk_player_update_audio (player);
  g_object_notify (G_OBJECT (player), "audio-enabled");
}

/**
 * swfdec_gtk_player_get_audio_enabled:
 * @player: a #SwfdecGtkPlayer
 *
 * Queries if audio playback for @player is enabled or not.
 *
 * Returns: %TRUE if audio playback is enabled
 **/
gboolean
swfdec_gtk_player_get_audio_enabled (SwfdecGtkPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_PLAYER (player), FALSE);

  return player->audio_enabled;
}

/**
 * swfdec_gtk_player_set_speed:
 * @player: a #SwfdecGtkPlayer
 * @speed: the new playback speed
 *
 * Sets the new playback speed. 1.0 means the default speed, bigger values
 * make playback happen faster. Audio will only play back if the speed is 
 * 1.0. Note that if the player is not playing when calling this function,
 * it will not automatically start.
 **/
void
swfdec_gtk_player_set_speed (SwfdecGtkPlayer *player, double speed)
{
  g_return_if_fail (SWFDEC_IS_GTK_PLAYER (player));
  g_return_if_fail (speed > 0.0);

  player->speed = speed;
  swfdec_gtk_player_update_audio (player);
  if (player->source)
    swfdec_iterate_source_set_speed (player->source, player->speed);
  g_object_notify (G_OBJECT (player), "speed");
}

/**
 * swfdec_gtk_player_get_speed:
 * @player: a #SwfdecGtkPlayer
 *
 * Queries the current playback speed. If the player is currently paused, it
 * will report the speed that it would use if playing.
 *
 * Returns: the current playback speed.
 **/
double
swfdec_gtk_player_get_speed (SwfdecGtkPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_PLAYER (player), FALSE);

  return player->speed;
}
