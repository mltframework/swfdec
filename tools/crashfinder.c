/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
 *               2007 Benjamin Otte <otte@gnome.org>
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

#include <swfdec/swfdec.h>

int
main (int argc, char **argv)
{
  GOptionContext *context;
  GError *err;
  SwfdecPlayer *player;
  SwfdecURL *url;
  guint i;
  cairo_surface_t *surface;
  cairo_t *cr;
  gboolean aborts;
  glong play_per_file = 30;
  glong max_per_file = 60;
  glong max_per_advance = 10;
  GTimer *timer;
  char **filenames = NULL;
  const GOptionEntry entries[] = {
    {
      "play-time", 'p', 0, G_OPTION_ARG_INT, &play_per_file,
      "How many seconds will be played from each file (default 30)", NULL
    },
    {
      "max-per-file", '\0', 0, G_OPTION_ARG_INT, &max_per_file,
      "Maximum runtime in seconds allowed for each file (default 60)", NULL
    },
    {
      "max-per-advance", '\0', 0, G_OPTION_ARG_INT, &max_per_advance,
      "Maximum runtime in seconds allowed for each advance (default 10)", NULL
    },
    {
      G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames,
      NULL, "<INPUT FILE> <OUTPUT FILE>"
    },
    {
      NULL
    }
  };

  // init
  swfdec_init ();

  // read command line params
  context = g_option_context_new ("Run a Flash file trying to crash Swfdec");
  g_option_context_add_main_entries (context, entries, NULL);

  if (g_option_context_parse (context, &argc, &argv, &err) == FALSE) {
    g_printerr ("Couldn't parse command-line options: %s\n", err->message);
    g_error_free (err);
    return 1;
  }

  if (filenames == NULL || g_strv_length (filenames) < 1) {
    g_printerr ("At least one input filename is required\n");
    return 1;
  }

  // make them milliseconds
  play_per_file *= 1000;
  max_per_file *= 1000;
  max_per_advance *= 1000;

  // create surface
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
  cr = cairo_create (surface);

  aborts = FALSE;
  for (i = 0; i < g_strv_length (filenames); i++)
  {
    glong played, advance, elapsed;

    g_print ("Running: %s\n", filenames[i]);

    // start timer
    timer = g_timer_new ();

    player = swfdec_player_new (NULL);
    url = swfdec_url_new_from_input (filenames[i]);
    swfdec_player_set_url (player, url);
    swfdec_url_free (url);

    // loop until we have played what we wanted, or timelimit is hit
    played = 0;
    elapsed = 0;
    while (played < play_per_file &&
	!swfdec_as_context_is_aborted (SWFDEC_AS_CONTEXT (player)))
    {
      elapsed = (glong)(g_timer_elapsed (timer, NULL) * 1000);
      if (elapsed >= max_per_file)
	break;
      swfdec_player_set_maximum_runtime (player,
	  MIN (max_per_advance, max_per_file - elapsed));

      advance = swfdec_player_get_next_event (player);
      if (advance == -1)
	break;
      played += swfdec_player_advance (player, advance);

      swfdec_player_render (player, cr, 0, 0, 0, 0);
    }

    if (elapsed >= max_per_file ||
	swfdec_as_context_is_aborted (SWFDEC_AS_CONTEXT (player))) {
      g_print ("*** Aborted ***\n");
      aborts = TRUE;
    }

    // clean up
    g_object_unref (player);
    g_timer_destroy (timer);
  }

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  if (aborts) {
    return 1;
  } else {
    return 0;
  }
}
