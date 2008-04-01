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
#include <swfdec/swfdec.h>

#include <swfdec-gtk/swfdec-gtk.h>

static GMainLoop *loop = NULL;

static void
set_title (GtkWindow *window, const SwfdecURL *url)
{
  char *title = swfdec_url_format_for_display (url);

  gtk_window_set_title (window, title);
  g_free (title);
}

static void
set_playing (SwfdecGtkWidget *widget, GParamSpec *pspec, gpointer player)
{
  swfdec_gtk_player_set_playing (player, TRUE);
}

static GtkWidget *
view_swf (SwfdecPlayer *player, gboolean use_image)
{
  GtkWidget *window, *widget;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_icon_name (GTK_WINDOW (window), "swfdec");
  widget = swfdec_gtk_widget_new (player);
  if (use_image)
    swfdec_gtk_widget_set_renderer (SWFDEC_GTK_WIDGET (widget), CAIRO_SURFACE_TYPE_IMAGE);
  gtk_container_add (GTK_CONTAINER (window), widget);
  g_signal_connect_swapped (window, "destroy", G_CALLBACK (g_main_loop_quit), loop);
  gtk_widget_show_all (window);
  g_signal_connect (widget, "notify::has-focus", G_CALLBACK (set_playing), player);

  return window;
}

static void
do_fscommand (SwfdecPlayer *player, const char *command, const char *value, gpointer window)
{
  if (g_str_equal (command, "quit")) {
    g_assert (loop);
    if (g_main_loop_is_running (loop)) {
      gtk_widget_destroy (window);
      g_main_loop_quit (loop);
    }
  }
  /* FIXME: add more */
}

static void
print_trace (SwfdecPlayer *player, const char *message, gpointer unused)
{
  g_print ("%s\n", message);
}

int 
main (int argc, char *argv[])
{
  int speed = 100;
  int max_runtime = 0;
  SwfdecPlayer *player;
  GError *error = NULL;
  gboolean use_image = FALSE, no_sound = FALSE;
  gboolean trace = FALSE, no_scripts = FALSE;
  gboolean redraws = FALSE, gc = FALSE;
  char *variables = NULL;
  GtkWidget *window;
  SwfdecURL *url;

  GOptionEntry options[] = {
    { "always-gc", 'g', 0, G_OPTION_ARG_NONE, &gc, "run the garbage collector as often as possible", NULL },
    { "image", 'i', 0, G_OPTION_ARG_NONE, &use_image, "use an intermediate image surface for drawing", NULL },
    { "max-runtime", 0, 0, G_OPTION_ARG_INT, &max_runtime, "maximum time scripts run before aborting the player", "SECS" },
    { "no-scripts", 0, 0, G_OPTION_ARG_NONE, &no_scripts, "don't execute scripts affecting the application", NULL },
    { "no-sound", 'n', 0, G_OPTION_ARG_NONE, &no_sound, "don't play sound", NULL },
    { "redraws", 'r', 0, G_OPTION_ARG_NONE, &redraws, "show redraw regions", NULL },
    { "speed", 0, 0, G_OPTION_ARG_INT, &speed, "replay speed (will deactivate sound)", "PERCENT" },
    { "trace", 't', 0, G_OPTION_ARG_NONE, &trace, "print trace output to stdout", NULL },
    { "variables", 'v', 0, G_OPTION_ARG_STRING, &variables, "variables to pass to player", "VAR=NAME[&VAR=NAME..]" },
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

  swfdec_init ();

  if (argc < 2) {
    g_printerr ("Usage: %s [OPTIONS] filename\n", argv[0]);
    return 1;
  }
  
  loop = g_main_loop_new (NULL, TRUE);
  player = swfdec_gtk_player_new (NULL);
  /* this allows the player to continue fine when running in gdb */
  if (max_runtime >= 0)
    swfdec_player_set_maximum_runtime (player, max_runtime * 1000);
  if (gc)
    g_object_set (player, "memory-until-gc", (gulong) 0, NULL);
  if (trace)
    g_signal_connect (player, "trace", G_CALLBACK (print_trace), NULL);
  swfdec_gtk_player_set_speed (SWFDEC_GTK_PLAYER (player), speed / 100.);

  if (no_sound)
    swfdec_gtk_player_set_audio_enabled (SWFDEC_GTK_PLAYER (player), FALSE);

  window = view_swf (player, use_image);
  if (redraws)
    gdk_window_set_debug_updates (TRUE);

  if (!no_scripts)
    g_signal_connect (player, "fscommand", G_CALLBACK (do_fscommand), window);
  
  if (variables != NULL)
    swfdec_player_set_variables (player, variables);
  url = swfdec_url_new_from_input (argv[1]);
  swfdec_player_set_url (player, url);
  set_title (GTK_WINDOW (window), url);
  swfdec_gtk_player_set_missing_plugins_window (SWFDEC_GTK_PLAYER (player),
      window->window);
  swfdec_url_free (url);

  if (g_main_loop_is_running (loop))
    g_main_loop_run (loop);

  g_object_unref (player);
  g_main_loop_unref (loop);
  loop = NULL;
  player = NULL;
  return 0;
}

