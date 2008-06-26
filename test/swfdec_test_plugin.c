/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_test_plugin.h"
#include <swfdec/swfdec.h>
#include "swfdec_test_test.h"
#include "swfdec_test_swfdec_socket.h"

static void
swfdec_test_plugin_swfdec_advance (SwfdecTestPlugin *plugin, unsigned int msecs)
{
  while (plugin->data && msecs > 0) {
    long next_event = swfdec_player_get_next_event (plugin->data);
    if (next_event < 0)
      break;
    next_event = MIN (next_event, (long) msecs);
    msecs -= swfdec_player_advance (plugin->data, next_event);
  }
  while (plugin->data && swfdec_player_get_next_event (plugin->data) == 0) {
    swfdec_player_advance (plugin->data, 0);
  }
}

static void
swfdec_test_plugin_swfdec_screenshot (SwfdecTestPlugin *plugin, unsigned char *data,
    guint x, guint y, guint width, guint height)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  guint background;

  surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32, 
      width, height, width * 4);
  cr = cairo_create (surface);
  background  = swfdec_player_get_background_color (plugin->data);
  cairo_set_source_rgb (cr, 
      ((background >> 16) & 0xFF) / 255.0,
      ((background >> 8) & 0xFF) / 255.0,
      (background & 0xFF) / 255.0);
  cairo_paint (cr);

  cairo_translate (cr, -x, -y);
  swfdec_player_render (plugin->data, cr, x, y, width, height);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void
swfdec_test_plugin_swfdec_mouse_move (SwfdecTestPlugin *plugin, double x, double y)
{
  swfdec_player_mouse_move (plugin->data, x, y);
}

static void
swfdec_test_plugin_swfdec_mouse_press (SwfdecTestPlugin *plugin, double x, double y, guint button)
{
  swfdec_player_mouse_press (plugin->data, x, y, button);
}

static void
swfdec_test_plugin_swfdec_mouse_release (SwfdecTestPlugin *plugin, double x, double y, guint button)
{
  swfdec_player_mouse_release (plugin->data, x, y, button);
}

static void
swfdec_test_plugin_swfdec_finish (SwfdecTestPlugin *plugin)
{
  if (plugin->data) {
    g_object_unref (plugin->data);
    plugin->data = NULL;
  }
}

static void
swfdec_test_plugin_swfdec_trace (SwfdecPlayer *player, const char *message, 
    SwfdecTestPlugin *plugin)
{
  plugin->trace (plugin, message);
}

static void
swfdec_test_plugin_swfdec_launch (SwfdecPlayer *player, const char *url,
    const char *target, SwfdecBuffer *data, guint header_count,
    const char **header_names, const char **header_values,
    SwfdecTestPlugin *plugin)
{
  plugin->launch (plugin, url);
}

static void
swfdec_test_plugin_swfdec_fscommand (SwfdecPlayer *player, const char *command, 
    const char *para, SwfdecTestPlugin *plugin)
{
  if (g_ascii_strcasecmp (command, "quit") == 0) {
    plugin->quit (plugin);
    swfdec_test_plugin_swfdec_finish (plugin);
  }
}

static void
swfdec_test_plugin_swfdec_notify (SwfdecPlayer *player, GParamSpec *pspec, SwfdecTestPlugin *plugin)
{
  if (g_str_equal (pspec->name, "default-width") ||
      g_str_equal (pspec->name, "default-height")) {
    swfdec_player_get_default_size (player, &plugin->width, &plugin->height);
  } else if (g_str_equal (pspec->name, "rate")) {
    plugin->rate = swfdec_player_get_rate (player) * 256;
  }
}

void
swfdec_test_plugin_swfdec_new (SwfdecTestPlugin *plugin)
{
  static const GTimeVal the_beginning = { 1035840244, 123 };
  SwfdecPlayer *player;
  SwfdecURL *url;

  plugin->advance = swfdec_test_plugin_swfdec_advance;
  plugin->screenshot = swfdec_test_plugin_swfdec_screenshot;
  plugin->mouse_move = swfdec_test_plugin_swfdec_mouse_move;
  plugin->mouse_press = swfdec_test_plugin_swfdec_mouse_press;
  plugin->mouse_release = swfdec_test_plugin_swfdec_mouse_release;
  plugin->finish = swfdec_test_plugin_swfdec_finish;
  plugin->data = player = g_object_new (SWFDEC_TYPE_PLAYER, "random-seed", 0,
      "loader-type", SWFDEC_TYPE_FILE_LOADER, "socket-type", SWFDEC_TYPE_TEST_SWFDEC_SOCKET,
      "max-runtime", 0, "start-time", &the_beginning, "allow-fullscreen", TRUE,
      NULL);

  g_object_set_data (G_OBJECT (player), "plugin", plugin);
  g_signal_connect (player, "fscommand", G_CALLBACK (swfdec_test_plugin_swfdec_fscommand), plugin);
  g_signal_connect (player, "trace", G_CALLBACK (swfdec_test_plugin_swfdec_trace), plugin);
  g_signal_connect (player, "launch", G_CALLBACK (swfdec_test_plugin_swfdec_launch), plugin);
  g_signal_connect (player, "notify", G_CALLBACK (swfdec_test_plugin_swfdec_notify), plugin);
  url = swfdec_url_new_from_input (plugin->filename);
  swfdec_player_set_url (player, url);
  swfdec_url_free (url);
  while (swfdec_player_get_next_event (player) == 0)
    swfdec_player_advance (player, 0);
}
