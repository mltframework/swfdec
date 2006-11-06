#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <libswfdec/swfdec.h>

static gboolean
audio_diff (SwfdecBuffer *compare, SwfdecBuffer *buffers[11])
{
  guint i, len;
  guchar *data;
  
  len = 0;
  for (i = 0; i < 11; i++) {
    len += buffers[i]->length;
  }
  if (compare->length != len) {
    g_print ("  ERROR: lengths don't match (is %u, should be %u)\n", 
	len, compare->length);
    return FALSE;
  }
  data = compare->data;
  for (i = 0; i < 11; i++) {
    if (memcmp (data, buffers[i]->data, buffers[i]->length)) {
      g_print ("  ERROR: data mismatch in buffer %u\n", i);
      return FALSE;
    }
  }
  return TRUE;
}

static gboolean
run_test (const char *filename)
{
  SwfdecPlayer *player;
  SwfdecBuffer *buffers[11];
  SwfdecBuffer *compare;
  guint i;
  GError *error = NULL;
  char *str;

  g_print ("Testing %s:\n", filename);
  player = swfdec_player_new_from_file (filename, &error);
  if (player == NULL) {
    g_print ("  ERROR: %s\n", error->message);
    return FALSE;
  }

  /* FIXME: Make the number of iterations configurable? */
  for (i = 0; i < 10; i++) {
    buffers[i] = swfdec_player_render_audio_to_buffer (player);
    swfdec_player_iterate (player);
  }
  buffers[10] = swfdec_player_render_audio_to_buffer (player);
  g_object_unref (player);
  player = NULL;

  str = g_strdup_printf ("%s.raw", filename);
  compare = swfdec_buffer_new_from_file (str, &error);
  if (compare == NULL) {
    g_print ("  ERROR: %s\n", error->message);
    g_error_free (error);
    g_free (str);
    return FALSE;
  }
  if (!audio_diff (compare, buffers)) {
    for (i = 0; i < 11; i++) {
      swfdec_buffer_unref (buffers[i]);
      buffers[i] = NULL;
    }
    swfdec_buffer_unref (compare);
    return FALSE;
  }
  for (i = 0; i < 11; i++) {
    swfdec_buffer_unref (buffers[i]);
    buffers[i] = NULL;
  }
  swfdec_buffer_unref (compare);
  g_print ("  OK\n");
  return TRUE;
}

int
main (int argc, char **argv)
{
  guint failed_tests = 0;

  swfdec_init ();

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

