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
#include <stdlib.h>
#include <string.h>
#include <libswfdec/swfdec.h>
#include "swfdec_interaction.h"

typedef struct _Test Test;
struct _Test {
  char *	filename;		/* name of the file to be tested */
  char *	output;			/* test result */
  gboolean	success;		/* TRUE if test was successful, FALSE on error */
  GMutex *	mutex;			/* NULL or mutex for protecting output */
  GCond *	cond;			/* NULL or cond to signal after setting output */
};

static Test *
test_new (char *filename)
{
  Test *test;

  test = g_slice_new0 (Test);
  test->filename = filename;
  return test;
}

static void
test_free (Test *test)
{
  g_free (test->filename);
  g_free (test->output);
  g_slice_free (Test, test);
}

static int
test_compare (gconstpointer a, gconstpointer b)
{
  const Test *ta = (const Test *) a;
  const Test *tb = (const Test *) b;

  return strcmp (ta->filename, tb->filename);
}

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

static void
run_test (gpointer testp, gpointer unused)
{
  Test *test = testp;
  SwfdecLoader *loader;
  SwfdecPlayer *player;
  SwfdecBuffer *buffer;
  guint time_left;
  char *str;
  GString *string, *output;
  GError *error = NULL;
  gboolean quit = FALSE;
  SwfdecInteraction *inter;

  output = g_string_new ("");
  g_string_append_printf (output, "Testing %s:\n", test->filename);
  loader = swfdec_file_loader_new (test->filename);
  if (loader->error) {
    g_string_append_printf (output, "  ERROR: %s\n", loader->error);
    goto fail;
  }
  string = g_string_new ("");
  player = swfdec_player_new ();
  g_signal_connect (player, "trace", G_CALLBACK (trace_cb), string);
  g_signal_connect (player, "fscommand", G_CALLBACK (fscommand_cb), &quit);
  swfdec_player_set_loader (player, loader);
  if (!swfdec_player_is_initialized (player)) {
    g_string_append_printf (output, "  ERROR: player is not initialized\n");
    g_object_unref (player);
    goto fail;
  }
  str = g_strdup_printf ("%s.act", test->filename);
  if (g_file_test (str, G_FILE_TEST_EXISTS)) {
    inter = swfdec_interaction_new_from_file (str, &error);
    if (inter == NULL) {
      g_string_append_printf (output, "  ERROR: %s\n", error->message);
      g_object_unref (player);
      g_error_free (error);
      g_free (str);
      goto fail;
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

  str = g_strdup_printf ("%s.trace", test->filename);
  buffer = swfdec_buffer_new_from_file (str, &error);
  if (buffer == NULL) {
    g_string_append_printf (output, "  ERROR: %s\n", error->message);
    g_error_free (error);
    g_string_free (string, TRUE);
    g_free (str);
    goto fail;
  }
  if (string->len != buffer->length ||
      memcmp (buffer->data, string->str, buffer->length) != 0) {
    g_string_append (output, "  ERROR: unexpected trace output\n");
    if (g_file_set_contents ("tmp", string->str, string->len, NULL)) {
      const char *command[] = { "diff", "-u", str, "tmp", NULL };
      char *result;
      if (!g_spawn_sync (NULL, (char **) command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	  &result, NULL, NULL, &error)) {
	g_string_append_printf (output, 
	    "  ERROR: Could not spawn diff to compare the results: %s\n", 
	    error->message);
	g_error_free (error);
      } else {
	g_string_append (output, result);
      }
    }
    g_string_free (string, TRUE);
    swfdec_buffer_unref (buffer);
    g_free (str);
    goto fail;
  }
  g_free (str);
  g_string_free (string, TRUE);
  swfdec_buffer_unref (buffer);
  g_string_append (output, "  OK\n");
  test->success = TRUE;
fail:
  if (test->mutex)
    g_mutex_lock (test->mutex);
  test->output = g_string_free (output, FALSE);
  if (test->mutex) {
    g_cond_signal (test->cond);
    g_mutex_unlock (test->mutex);
  }
}

int
main (int argc, char **argv)
{
  GList *walk, *tests = NULL;
  GString *failed_tests;
  guint failures = 0;
  GThreadPool *pool;
  GError *error = NULL;

  g_thread_init (NULL);
  swfdec_init ();
  failed_tests = g_string_new ("");

  /* collect all tests into the tests list */
  if (argc > 1) {
    int i;
    for (i = 1; i < argc; i++) {
      tests = g_list_append (tests, test_new (g_strdup (argv[i])));
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
      tests = g_list_append (tests, test_new (name));
    }
    g_dir_close (dir);
  }

  /* sort the tests by filename */
  tests = g_list_sort (tests, test_compare);

  /* run them and put failed ones in failed_tests */
  if (g_getenv ("SWFDEC_TEST_THREADS")) {
    pool = g_thread_pool_new (run_test, NULL, -1, FALSE, &error);
    if (pool == NULL) {
      g_print ("  WARNING: Could not start thread pool: %s\n", error->message);
      g_print ("  WARNING: testing unthreaded\n");
      g_error_free (error);
      error = NULL;
    }
  } else {
    pool = NULL;
  }
  if (pool == NULL) {
    for (walk = tests; walk; walk = walk->next) {
      Test *test = walk->data;
      
      run_test (test, NULL);
      g_print ("%s", test->output);
      if (!test->success) {
	failures++;
	g_string_append_printf (failed_tests, 
	    "          %s\n", test->filename);
      }
      test_free (test);
    }
  } else {
    GMutex *mutex = g_mutex_new ();
    GCond *cond = g_cond_new ();
    for (walk = tests; walk; walk = walk->next) {
      Test *test = walk->data;
      test->mutex = mutex;
      test->cond = cond;
      g_thread_pool_push (pool, test, &error);
      if (error) {
	/* huh? */
	g_assert_not_reached ();
	g_error_free (error);
	error = NULL;
      }
    }
    g_mutex_lock (mutex);
    for (walk = tests; walk; walk = walk->next) {
      Test *test = walk->data;
      while (test->output == NULL)
	g_cond_wait (cond, mutex);
      g_print (test->output);
      if (!test->success) {
	failures++;
	g_string_append_printf (failed_tests, 
	    "          %s\n", test->filename);
      }
      test_free (test);
    }
    g_mutex_unlock (mutex);
    g_cond_free (cond);
    g_mutex_free (mutex);
  }
  g_list_free (tests);

  /* report failures and exit */
  if (failures > 0) {
    g_print ("\nFAILURES: %u\n", failures);
    g_print ("%s", failed_tests->str);
    g_string_free (failed_tests, TRUE);
    return EXIT_FAILURE;
  } else {
    g_print ("\nEVERYTHING OK\n");
    g_string_free (failed_tests, TRUE);
    return EXIT_SUCCESS;
  }
}

