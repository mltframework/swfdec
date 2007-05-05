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

#include <libswfdec-gtk/swfdec-gtk.h>
#if HAVE_GNOMEVFS
#include <libgnomevfs/gnome-vfs.h>
#endif

#include "swfdec_slow_loader.h"

static void
set_title (GtkWindow *window, const char *filename)
{
  char *name = g_filename_display_basename (filename);
  char *title = g_strdup_printf ("%s : Swfplay", name);

  g_free (name);
  gtk_window_set_title (window, title);
  g_free (title);
}

static GtkWidget *
view_swf (SwfdecPlayer *player, double scale, gboolean use_image)
{
  GtkWidget *window, *widget;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  widget = swfdec_gtk_widget_new (player);
  swfdec_gtk_widget_set_scale (SWFDEC_GTK_WIDGET (widget), scale);
  if (use_image)
    swfdec_gtk_widget_set_renderer (SWFDEC_GTK_WIDGET (widget), CAIRO_SURFACE_TYPE_IMAGE);
  gtk_container_add (GTK_CONTAINER (window), widget);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (window);

  return window;
}

static void
print_trace (SwfdecPlayer *player, const char *message, gpointer unused)
{
  g_print ("%s\n", message);
}

int 
main (int argc, char *argv[])
{
  int ret = 0;
  int delay = 0;
  int speed = 100;
  double scale;
  SwfdecLoader *loader;
  SwfdecPlayer *player;
  GError *error = NULL;
  gboolean use_image = FALSE, no_sound = FALSE;
  gboolean trace = FALSE;
  char *variables = NULL;
  GtkWidget *window;

  GOptionEntry options[] = {
    { "delay", 'd', 0, G_OPTION_ARG_INT, &delay, "make loading of resources take time", "SECS" },
    { "image", 'i', 0, G_OPTION_ARG_NONE, &use_image, "use an intermediate image surface for drawing", NULL },
    { "no-sound", 'n', 0, G_OPTION_ARG_NONE, &no_sound, "don't play sound", NULL },
    { "scale", 's', 0, G_OPTION_ARG_INT, &ret, "scale factor", "PERCENT" },
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

  scale = ret / 100.;
  swfdec_init ();

  if (argc < 2) {
    g_printerr ("Usage: %s [OPTIONS] filename\n", argv[0]);
    return 1;
  }

#if HAVE_GNOMEVFS
  {
    char *s;
    s = gnome_vfs_make_uri_from_shell_arg (argv[1]);
    loader = swfdec_gtk_loader_new (s);
    g_free (s);
  }
#else
  loader = swfdec_gtk_loader_new (argv[1]);
#endif
  if (loader->error) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], loader->error);
    g_object_unref (loader);
    return 1;
  }
  player = swfdec_gtk_player_new ();
  if (trace)
    g_signal_connect (player, "trace", G_CALLBACK (print_trace), NULL);
  
  if (delay) 
    loader = swfdec_slow_loader_new (loader, delay);

  swfdec_player_set_loader_with_variables (player, loader, variables);

  if (no_sound)
    swfdec_gtk_player_set_audio_enabled (SWFDEC_GTK_PLAYER (player), FALSE);

  swfdec_gtk_player_set_speed (SWFDEC_GTK_PLAYER (player), speed / 100.);
  swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (player), TRUE);

  window = view_swf (player, scale, use_image);
  set_title (GTK_WINDOW (window), argv[1]);

  gtk_main ();

  g_object_unref (player);
  player = NULL;
  return 0;
}

