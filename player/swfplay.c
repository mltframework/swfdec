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
#include <gtk/gtk.h>
#include <math.h>
#include <libswfdec/swfdec.h>

#include "swfdec_playback.h"
#include "swfdec_widget.h"

static gpointer playback;

static gboolean
iterate (gpointer player)
{
  swfdec_player_iterate (player);
  if (playback != NULL) {
    SwfdecBuffer *buffer = swfdec_player_render_audio_to_buffer (player);
    swfdec_playback_write (playback, buffer);
    swfdec_buffer_unref (buffer);
  }
  return TRUE;
}

static void
view_swf (SwfdecPlayer *player, double scale, gboolean use_image)
{
  GtkWidget *window, *widget;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  widget = swfdec_widget_new (player);
  swfdec_widget_set_scale (SWFDEC_WIDGET (widget), scale);
  swfdec_widget_set_use_image (SWFDEC_WIDGET (widget), use_image);
  gtk_container_add (GTK_CONTAINER (window), widget);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (window);
}

static void
play_swf (SwfdecPlayer *player)
{
  double rate;

  rate = swfdec_player_get_rate (player);
  if (rate == 0)
    return;

  if (playback == NULL) {
    guint timeout = g_timeout_add (1000 / rate, iterate, player);

    gtk_main ();
    g_source_remove (timeout);
  } else {
    gtk_main ();
  }
}

int 
main (int argc, char *argv[])
{
  int ret = 0;
  double scale;
  SwfdecPlayer *player;
  guint buffer_size = 0;
  GError *error = NULL;
  gboolean use_image = FALSE, no_sound = FALSE;

  GOptionEntry options[] = {
    { "scale", 's', 0, G_OPTION_ARG_INT, &ret, "scale factor", "PERCENT" },
    { "image", 'i', 0, G_OPTION_ARG_NONE, &use_image, "use an intermediate image surface for drawing", NULL },
    { "no-sound", 'n', 0, G_OPTION_ARG_NONE, &no_sound, "don't play sound", NULL },
    { NULL }
  };
  GOptionContext *ctx;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);

  if (error) {
    g_printerr ("Error parsing command line arguments: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  scale = ret / 100.;
  swfdec_init ();

  if (argc < 2) {
    g_printerr ("Usage: %s [OPTIONS] filename\n", argv[0]);
    return 1;
  }

  player = swfdec_player_new_from_file (argv[1], &error);
  if (player == NULL) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    g_error_free (error);
    return 1;
  }
  /* FIXME add smarter "not my file" detection */
  if (swfdec_player_get_rate (player) == 0) {
    g_printerr ("File \"%s\" is not a SWF file\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }

  view_swf (player, scale, use_image);

  if (no_sound) {
    playback = NULL;
  } else {
    double rate;

    rate = swfdec_player_get_rate (player);
    if (rate != 0)
      playback = swfdec_playback_open (iterate, player, G_USEC_PER_SEC / rate, &buffer_size);
    else
      playback = NULL;
  }
  if (buffer_size) {
    SwfdecBuffer *buffer;

    g_print ("using buffering of %u frames (%gs)\n", buffer_size, buffer_size / 44100.);
    swfdec_player_set_latency (player, buffer_size);
    buffer = swfdec_buffer_new ();
    buffer->length = buffer_size * 4;
    buffer->data = g_malloc0 (buffer->length);
    swfdec_player_render_audio (player, (gint16 *) buffer->data,
	0, buffer_size);
    swfdec_playback_write (playback, buffer);
    swfdec_buffer_unref (buffer);
  }
  play_swf (player);

  if (playback)
    swfdec_playback_close (playback);

  g_object_unref (player);
  player = NULL;
  return 0;
}

