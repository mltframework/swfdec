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
#include <math.h>
#include <string.h>
#include <libswfdec/swfdec.h>
#include "swfdec_interaction.h"

static void
trace_cb (SwfdecPlayer *player, const char *message, GString *string)
{
  g_string_append_printf (string, "%s\n", message);
}

static void
fscommand_cb (SwfdecPlayer *player, const char *command, const char *parameter, gpointer data)
{
  gboolean *quit = data;

  if (g_str_equal (command, "quit")) {
    *quit = TRUE;
  }
}

static gboolean
run_test (const char *filename)
{
  SwfdecLoader *loader;
  SwfdecPlayer *player;
  SwfdecBuffer *buffer;
  guint time_left;
  char *str;
  GString *string;
  GError *error = NULL;
  gboolean quit = FALSE;
  SwfdecInteraction *inter;

  g_print ("Testing %s:\n", filename);
  loader = swfdec_loader_new_from_file (filename);
  if (loader->error) {
    g_print ("  ERROR: %s\n", loader->error);
    return FALSE;
  }
  string = g_string_new ("");
  player = swfdec_player_new ();
  g_signal_connect (player, "trace", G_CALLBACK (trace_cb), string);
  g_signal_connect (player, "fscommand", G_CALLBACK (fscommand_cb), &quit);
  swfdec_player_set_loader (player, loader);
  if (!swfdec_player_is_initialized (player)) {
    g_print ("  ERROR: player is not initialized\n");
    g_object_unref (player);
    return FALSE;
  }
  str = g_strdup_printf ("%s.act", filename);
  if (g_file_test (str, G_FILE_TEST_EXISTS)) {
    inter = swfdec_interaction_new_from_file (str, &error);
    if (inter == NULL) {
      g_print ("  ERROR: %s\n", error->message);
      g_object_unref (player);
      g_error_free (error);
      g_free (str);
      return FALSE;
    }
    time_left = swfdec_interaction_get_duration (inter);
  } else {
    time_left = ceil (10000 / swfdec_player_get_rate (player));
    inter = NULL;
  }
  g_free (str);

  /* FIXME: Make the number of iterations configurable? */
  while (quit == FALSE) {
    /* FIXME: will not do 10 iterations if there's other stuff loaded */
    guint advance = swfdec_player_get_next_event (player);

    if (inter) {
      int t = swfdec_interaction_get_next_event (inter);
      g_assert (t >= 0);
      advance = MIN (advance, (guint) t);
    }
    if (advance > time_left)
      break;
    swfdec_player_advance (player, advance);
    if (inter)
      swfdec_interaction_advance (inter, player, advance);
    time_left -= advance;
  }
  g_signal_handlers_disconnect_by_func (player, trace_cb, string);
  g_object_unref (player);

  str = g_strdup_printf ("%s.trace", filename);
  buffer = swfdec_buffer_new_from_file (str, &error);
  if (buffer == NULL) {
    g_print ("  ERROR: %s\n", error->message);
    g_error_free (error);
    g_string_free (string, TRUE);
    g_free (str);
    return FALSE;
  }
  if (string->len != buffer->length ||
      memcmp (buffer->data, string->str, buffer->length) != 0) {
    g_print ("  ERROR: unexpected trace output\n");
    if (g_file_set_contents ("tmp", string->str, string->len, NULL)) {
      char *command[] = { "diff", "-u", (char *) str, "tmp", NULL };
      char *result;
      if (!g_spawn_sync (NULL, command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	  &result, NULL, NULL, &error)) {
	g_printerr ("  Couldn't spawn diff to compare the results: %s\n", error->message);
	g_error_free (error);
      } else {
	g_print ("%s", result);
      }
    }
    g_string_free (string, TRUE);
    swfdec_buffer_unref (buffer);
    g_free (str);
    return FALSE;
  }
  g_free (str);
  g_string_free (string, TRUE);
  swfdec_buffer_unref (buffer);
  g_print ("  OK\n");
  return TRUE;
}

int
main (int argc, char **argv)
{
  GList *failed_tests = NULL;

  swfdec_init ();

  if (argc > 1) {
    int i;
    for (i = 1; i < argc; i++) {
      if (!run_test (argv[i]))
	failed_tests = g_list_prepend (failed_tests, g_strdup (argv[i]));;
    }
  } else {
    GDir *dir;
    char *name;
    const char *path, *file;
    /* automake defines this */
    path = g_getenv ("srcdir");
    if (path == NULL)
      path = ".";
    dir = g_dir_open (path, 0, NULL);
    while ((file = g_dir_read_name (dir))) {
      if (!g_str_has_suffix (file, ".swf"))
	continue;
      name = g_build_filename (path, file, NULL);
      if (!run_test (name)) {
	failed_tests = g_list_prepend (failed_tests, name);
      } else {
	g_free (name);
      }
    }
    g_dir_close (dir);
  }

  if (failed_tests) {
    GList *walk;
    failed_tests = g_list_sort (failed_tests, (GCompareFunc) strcmp);
    g_print ("\nFAILURES: %u\n", g_list_length (failed_tests));
    for (walk = failed_tests; walk; walk = walk->next) {
      g_print ("          %s\n", (char *) walk->data);
      g_free (walk->data);
    }
    g_list_free (failed_tests);
    return 1;
  } else {
    g_print ("\nEVERYTHING OK\n");
    return 0;
  }
}

