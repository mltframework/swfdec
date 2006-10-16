#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <swfdec.h>
#include <swfdec_buffer.h>

static void
trace_cb (SwfdecDecoder *dec, const char *message, GString *string)
{
  g_string_append_printf (string, "%s\n", message);
}

static gboolean
run_test (const char *filename)
{
  SwfdecPlayer *player;
  SwfdecBuffer *buffer;
  guint i;
  GError *error = NULL;
  char *str;
  GString *string;

  g_print ("Testing %s:\n", filename);
  player = swfdec_player_new_from_file (filename, &error);
  if (player == NULL) {
    g_print ("  ERROR: %s\n", error->message);
    return FALSE;
  }
  string = g_string_new ("");
  g_signal_connect (player, "trace", G_CALLBACK (trace_cb), string);

  /* FIXME: Make the number of iterations configurable? */
  for (i = 0; i < 10; i++) {
    swfdec_player_iterate (player);
  }
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
      if (!g_spawn_sync (NULL, command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	  NULL, NULL, NULL, &error)) {
	g_printerr ("  Couldn't spawn diff to compare the results: %s\n", error->message);
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
  guint failed_tests = 0;

  if (argc > 1) {
    int i;
    for (i = 1; i < argc; i++) {
      if (!run_test (argv[i]))
	failed_tests++;
    }
  } else {
    GDir *dir;
    const char *file;
    dir = g_dir_open (".", 0, NULL);
    while ((file = g_dir_read_name (dir))) {
      if (!g_str_has_suffix (file, ".swf"))
	continue;
      if (!run_test (file))
	failed_tests++;
    }
    g_dir_close (dir);
  }

  if (failed_tests) {
    g_print ("\nFAILURES: %u\n", failed_tests);
  } else {
    g_print ("\nEVERYTHING OK\n");
  }
  return failed_tests;
}

