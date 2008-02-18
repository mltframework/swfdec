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

#ifdef HAVE_GST
#include <gst/pbutils/pbutils.h>
#endif

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "swfdec-gtk/swfdec_gtk_loader.h"
#include "swfdec-gtk/swfdec_gtk_player.h"
#include "swfdec-gtk/swfdec_gtk_socket.h"
#include "swfdec-gtk/swfdec_gtk_system.h"
#include "swfdec-gtk/swfdec_playback.h"
#include "swfdec-gtk/swfdec_source.h"

struct _SwfdecGtkPlayerPrivate
{
  GSource *		source;		/* source if playing, NULL otherwise */
  SwfdecPlayback *	playback;	/* audio playback object */
  gboolean		audio_enabled;	/* TRUE if audio should be played */
  double		speed;		/* desired playback speed */

  /* missing plugins */
  GdkWindow *		missing_plugins_window; /* window used for displaying missing plugins info */
#ifdef HAVE_GST
  GstInstallPluginsContext *missing_plugins_context; /* context used during install */
#endif
  gboolean		needs_resume;	/* restart playback after plugin install is done? */
};

enum {
  PROP_0,
  PROP_PLAYING,
  PROP_AUDIO,
  PROP_SPEED,
  PROP_MISSING_PLUGINS_WINDOW
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

/*** MISSING PLUGINS ***/

#ifdef HAVE_GST
static void
swfdec_gtk_player_missing_plugins_done (GstInstallPluginsReturn result, gpointer data)
{
  SwfdecGtkPlayer *player = data;
  SwfdecGtkPlayerPrivate *priv = player->priv;

  gst_update_registry ();
  if (priv->needs_resume)
    swfdec_gtk_player_set_playing (player, TRUE);
}
#endif

static void
swfdec_gtk_player_missing_plugins (SwfdecPlayer *player, const char **details)
{
  SwfdecGtkPlayerPrivate *priv = SWFDEC_GTK_PLAYER (player)->priv;

  /* That should only happen if users don't listen and then we don't listen either */
#ifdef HAVE_GST
  if (priv->missing_plugins_context)
    return;
#endif
  /* no automatic install desired */
  if (priv->missing_plugins_window == NULL)
    return;

#ifdef HAVE_GST
  {
    GstInstallPluginsReturn result;
    priv->missing_plugins_context = gst_install_plugins_context_new ();
#ifdef GDK_WINDOWING_X11
    gst_install_plugins_context_set_xid (priv->missing_plugins_context,
	GDK_DRAWABLE_XID (priv->missing_plugins_window));
#endif
    result = gst_install_plugins_async ((char **) details, priv->missing_plugins_context,
	swfdec_gtk_player_missing_plugins_done, player);
    if (result == GST_INSTALL_PLUGINS_STARTED_OK) {
      if (swfdec_gtk_player_get_playing (SWFDEC_GTK_PLAYER (player))) {
	swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (player), FALSE);
	priv->needs_resume = TRUE;
      }
    } else {
      /* FIXME: error handling */
    }
  }
#endif
}

/*** SWFDEC_GTK_PLAYER ***/

G_DEFINE_TYPE (SwfdecGtkPlayer, swfdec_gtk_player, SWFDEC_TYPE_PLAYER)

static void
swfdec_gtk_player_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecGtkPlayerPrivate *priv = SWFDEC_GTK_PLAYER (object)->priv;
  
  switch (param_id) {
    case PROP_PLAYING:
      g_value_set_boolean (value, priv->source != NULL);
      break;
    case PROP_AUDIO:
      g_value_set_boolean (value, priv->audio_enabled);
      break;
    case PROP_SPEED:
      g_value_set_double (value, priv->speed);
      break;
    case PROP_MISSING_PLUGINS_WINDOW:
      g_value_set_object (value, G_OBJECT (priv->missing_plugins_window));
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
    case PROP_MISSING_PLUGINS_WINDOW:
      swfdec_gtk_player_set_missing_plugins_window (player,
	  GDK_WINDOW (g_value_get_object (value)));
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
  swfdec_gtk_player_set_missing_plugins_window (player, NULL);
  g_assert (player->priv->playback == NULL);

  G_OBJECT_CLASS (swfdec_gtk_player_parent_class)->dispose (object);
}

static void
swfdec_gtk_player_class_init (SwfdecGtkPlayerClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecPlayerClass *player_class = SWFDEC_PLAYER_CLASS (g_class);

  g_type_class_add_private (g_class, sizeof (SwfdecGtkPlayerPrivate));

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
  g_object_class_install_property (object_class, PROP_MISSING_PLUGINS_WINDOW,
      g_param_spec_object ("missing-plugins-window", "missing plugins window", 
	  "window on which to do autmatic missing plugins installation",
	  GDK_TYPE_WINDOW, G_PARAM_READWRITE));

  player_class->missing_plugins = swfdec_gtk_player_missing_plugins;
}

static void
swfdec_gtk_player_init (SwfdecGtkPlayer * player)
{
  SwfdecGtkPlayerPrivate *priv;

  player->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (player, SWFDEC_TYPE_GTK_PLAYER, SwfdecGtkPlayerPrivate);

  priv->speed = 1.0;
  priv->audio_enabled = TRUE;
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
  SwfdecSystem *sys;
  
  swfdec_init ();

  sys = swfdec_gtk_system_new (NULL);
  player = g_object_new (SWFDEC_TYPE_GTK_PLAYER, 
      "loader-type", SWFDEC_TYPE_GTK_LOADER, "socket-type", SWFDEC_TYPE_GTK_SOCKET,
      "system", sys, "debugger", debugger, NULL);
  g_object_unref (sys);

  return player;
}

static void
swfdec_gtk_player_update_audio (SwfdecGtkPlayer *player)
{
  SwfdecGtkPlayerPrivate *priv = player->priv;
  gboolean should_play = priv->audio_enabled;
  
  should_play &= (priv->source != NULL);
  should_play &= (priv->speed == 1.0);

  if (should_play && priv->playback == NULL) {
    priv->playback = swfdec_playback_open (SWFDEC_PLAYER (player),
	g_main_context_default ());
  } else if (!should_play && priv->playback != NULL) {
    swfdec_playback_close (priv->playback);
    priv->playback = NULL;
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
  SwfdecGtkPlayerPrivate *priv;

  g_return_if_fail (SWFDEC_IS_GTK_PLAYER (player));

  priv = player->priv;
  if (playing && priv->source == NULL) {
    priv->source = swfdec_iterate_source_new (SWFDEC_PLAYER (player), priv->speed);
    g_source_attach (priv->source, NULL);
  } else if (!playing && priv->source != NULL) {
    g_source_destroy (priv->source);
    g_source_unref (priv->source);
    priv->source = NULL;
  } else {
    return;
  }
  priv->needs_resume = FALSE;
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

  return player->priv->source != NULL;
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

  if (player->priv->audio_enabled == enabled)
    return;
  player->priv->audio_enabled = enabled;
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

  return player->priv->audio_enabled;
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

  player->priv->speed = speed;
  swfdec_gtk_player_update_audio (player);
  if (player->priv->source)
    swfdec_iterate_source_set_speed (player->priv->source, player->priv->speed);
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

  return player->priv->speed;
}

/**
 * swfdec_gtk_player_set_missing_plugins_window:
 * @player: a #SwfdecGtkPlayer
 * @window: the window to use for popping up codec install dialogs or %NULL
 *
 * Sets a given #GdkWindow to be used as the reference window when popping up
 * automatic codec install dialogs. Automatic codec install happens when Swfdec
 * cannot find a GStreamer plugin to play back a given media file. The player
 * will automaticcaly pause when this happens to allow the plugin install to 
 * finish and will resume playback after the codec install has completed.
 * You can use %NULL to disable this feature. It is disable by default.
 * Note that this function takes a #GdkWindow, not a #GtkWindow, as its 
 * argument. This makes it slightly more inconvenient to use, as you need to 
 * call gtk_widget_show() on your #GtkWindow before having access to its 
 * #GdkWindow, but it allows automatic plugin installatio even when your 
 * application does not have a window. You can use gdk_get_default_root_window()
 * to obtain a default window in that case.
 * For details about automatic codec install, see the GStreamer documentation,
 * in particular the function gst_install_plugins_async(). If Swfdec was 
 * compiled without GStreamer support, this function will have no effect.
 **/
void
swfdec_gtk_player_set_missing_plugins_window (SwfdecGtkPlayer *player,
    GdkWindow *window)
{
  SwfdecGtkPlayerPrivate *priv;

  g_return_if_fail (SWFDEC_IS_GTK_PLAYER (player));
  g_return_if_fail (window == NULL || GDK_IS_WINDOW (window));

  priv = player->priv;
  if (priv->missing_plugins_window)
    g_object_unref (priv->missing_plugins_window);

  priv->missing_plugins_window = window;

  if (window)
    g_object_ref (window);
  g_object_notify (G_OBJECT (player), "missing-plugins-window");
}

/**
 * swfdec_gtk_player_get_missing_plugins_window:
 * @player: a #SwfdecPlayer
 *
 * Gets the window used for automatic codec install. See 
 * swfdec_gtk_player_set_missing_plugins_window() for details.
 *
 * Returns: the #GdkWindow used as parent for showing missing plugin dialogs or
 *          %NULL if automatic codec install is disabled.
 **/
GdkWindow *
swfdec_gtk_player_get_missing_plugins_window (SwfdecGtkPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_PLAYER (player), NULL);

  return player->priv->missing_plugins_window;
}

