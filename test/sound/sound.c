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
#include <string.h>
#include <swfdec/swfdec.h>

/**
 * audio_diff:
 * @compare: the buffer we rendered in 44.1kHz stereo 16bit
 * @original: the original buffer in 5.5kHz stereo 16bit
 * @filename: name of the file where @original came from
 *
 * Compares the 2 buffers for every 5.5kHz and complains with a useful 
 * error message if they don't match.
 *
 * Returns: TRUE if the 2 files are equal
 **/
static gboolean
audio_diff (SwfdecBuffer *compare, SwfdecBuffer *original, const char *filename)
{
  guint i, length;
  gint16 *comp_data, *comp_end, *org_data;
  
  /* must hold since we are rendering it */
  g_assert (compare->length % 4 == 0);
  if (original->length % 4 != 0) {
    g_print ("  ERROR: %s: filesize (%"G_GSIZE_FORMAT" bytes) not multiple of 4\n", filename,
	original->length);
    return FALSE;
  }
  length = original->length / 4;
  comp_data = (gint16 *) compare->data;
  comp_end = (gint16 *) (compare->data + compare->length);
  org_data = (gint16 *) original->data;
  comp_data += 14;
  for (i = 0; i < length && comp_data < comp_end; i++) {
    /* original data is little endian */
    if (*comp_data != GINT16_FROM_LE (*org_data)) {
      g_print ("  ERROR: %s: data mismatch at left channel for sample %u (is %04hX, should be %04hX)\n",
	  filename, i, *comp_data, GINT16_FROM_LE (*org_data));
      goto dump;
    }
    comp_data++;
    org_data++;
    if (*comp_data != GINT16_FROM_LE (*org_data)) {
      g_print ("  ERROR: %s: data mismatch at right channel for sample %u (is %04hX, should be %04hX)\n",
	  filename, i, *comp_data, GINT16_FROM_LE (*org_data));
      goto dump;
    }
    comp_data += 15;
    org_data++;
  }
  if (i < length) {
    g_print ("  ERROR: %s: not enough data: Should be %u 5.5kHz samples, but is only %u samples",
	filename, length, i);
    goto dump;
  }
  while (comp_data < comp_end) {
    if (comp_data[0] != 0 || comp_data[1] != 0) {
      g_print ("  ERROR: %s: leftover data should be 0, but sample %u is %04hX, %04hX\n",
	  filename, i, comp_data[0], comp_data[1]);
      goto dump;
    }
    i++;
    comp_data += 16;
  }
  return TRUE;
dump:
  if (g_getenv ("SWFDEC_TEST_DUMP")) {
    GError *error = NULL;
    char *dump = g_strdup_printf ("%s.dump", filename);
    /* convert to LE */
    comp_data = (gint16 *) compare->data;
    for (i = 0; i < compare->length / 64; i++) {
      comp_data[2 * i] = GINT16_TO_LE (comp_data[i * 16 + 14]);
      comp_data[2 * i + 1] = GINT16_TO_LE (comp_data[i * 16 + 15]);
    }
    if (!g_file_set_contents (dump, (char *) compare->data, compare->length, &error)) {
      g_print ("  ERROR: failed to dump contents: %s\n", error->message);
      g_error_free (error);
    }
    g_free (dump);
  }
  return FALSE;
}

typedef struct {
  SwfdecAudio *		audio;
  char *		name;
  SwfdecBufferQueue *	queue;
} TestStream;

typedef struct {
  const char *	filename;
  GList *	files;
  GList *	streams;
  guint		current_frame;
  guint		current_frame_audio;
  gboolean	success;
} TestData;

static void
audio_added (SwfdecPlayer *player, SwfdecAudio *audio, TestData *data)
{
  char *basename = g_path_get_basename (data->filename);
  char *name = g_strdup_printf ("%s.%u.%u.raw", basename, data->current_frame, data->current_frame_audio);
  GList *found = g_list_find_custom (data->files, name, (GCompareFunc) strcmp);

  g_free (basename);
  if (found == NULL) {
    g_print ("  ERROR: %s wasn't found\n", name);
    data->success = FALSE;
  } else {
    TestStream *stream = g_new0 (TestStream, 1);
    char *dirname;

    dirname = g_path_get_dirname (data->filename);
    stream->audio = audio;
    stream->name = g_build_filename (dirname, found->data, NULL);
    stream->queue = swfdec_buffer_queue_new ();
    data->files = g_list_delete_link (data->files, found);
    data->streams = g_list_prepend (data->streams, stream);
    g_free (dirname);
  }
  g_free (name);
}

static gboolean
finish_stream (TestStream *stream)
{
  SwfdecBuffer *buffer, *file;
  GError *error = NULL;
  gboolean ret = TRUE;

  buffer = swfdec_buffer_queue_pull (stream->queue, swfdec_buffer_queue_get_depth (stream->queue));
  swfdec_buffer_queue_unref (stream->queue);
  file = swfdec_buffer_new_from_file (stream->name, &error);
  if (file) {
    ret = audio_diff (buffer, file, stream->name);
    swfdec_buffer_unref (file);
  } else {
    g_print ("  ERROR: %s\n", error->message);
    g_error_free (error);
    ret = FALSE;
  }
  swfdec_buffer_unref (buffer);
  g_free (stream->name);
  g_free (stream);
  return ret;
}

static void
audio_removed (SwfdecPlayer *player, SwfdecAudio *audio, TestData *data)
{
  TestStream *stream = NULL;
  GList *walk;

  for (walk = data->streams; walk; walk = walk->next) {
    stream = walk->data;
    if (stream->audio == audio)
      break;
    stream = NULL;
  }
  if (stream) {
    data->streams = g_list_remove (data->streams, stream);
    data->success &= finish_stream (stream);
  }
}

static void
render_all_streams (SwfdecPlayer *player, guint msecs, guint n_samples, TestData *data)
{
  GList *walk;
  
  for (walk = data->streams; walk; walk = walk->next) {
    TestStream *stream = walk->data;
    SwfdecBuffer *buffer = swfdec_buffer_new0 (n_samples * 4);
    swfdec_audio_render (stream->audio, (gint16 *) buffer->data, 0, n_samples);
    swfdec_buffer_queue_push (stream->queue, buffer);
  }
}

static gboolean
run_test (const char *filename)
{
  SwfdecURL *url;
  SwfdecPlayer *player = NULL;
  guint i;
  long msecs;
  GError *error = NULL;
  char *dirname, *basename;
  const char *name;
  GDir *dir;
  GList *walk;
  TestData data = { filename, NULL, NULL, 0, 0, TRUE };

  g_print ("Testing %s:\n", filename);
  dirname = g_path_get_dirname (filename);
  basename = g_path_get_basename (filename);
  dir = g_dir_open (dirname, 0, &error);
  if (!dir) {
    g_print ("  ERROR: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }
  while ((name = g_dir_read_name (dir))) {
    if (!g_str_has_prefix (name, basename))
      continue;
    if (!g_str_has_suffix (name, ".raw"))
      continue;
    data.files = g_list_prepend (data.files, g_strdup (name));
  }
  g_dir_close (dir);
  g_free (dirname);
  g_free (basename);

  player = swfdec_player_new (NULL);
  url = swfdec_url_new_from_input (filename);
  g_signal_connect (player, "audio-added", G_CALLBACK (audio_added), &data);
  g_signal_connect (player, "audio-removed", G_CALLBACK (audio_removed), &data);
  g_signal_connect (player, "advance", G_CALLBACK (render_all_streams), &data);
  swfdec_player_set_url (player, url);
  swfdec_url_free (url);

  for (i = 0; i < 10; i++) {
    data.current_frame++;
    data.current_frame_audio = 0;
    msecs = swfdec_player_get_next_event (player);
    if (msecs < 0)
      goto error;
    swfdec_player_advance (player, msecs);
  }
  g_object_unref (player);
  for (walk = data.streams; walk; walk = walk->next) {
    data.success &= finish_stream (walk->data);
  }
  g_list_free (data.streams);
  if (data.files) {
    g_print ("  ERROR: streams not played:\n");
    for (walk = data.files; walk; walk = walk->next) {
      g_print ("         %s\n", (char *) walk->data);
      g_free (walk->data);
    }
    g_list_free (data.files);
  }
  if (data.success) {
    g_print ("  OK\n");
    return TRUE;
  } else {
    return FALSE;
  }

error:
  if (player)
    g_object_unref (player);
  g_list_foreach (data.files, (GFunc) g_free, NULL);
  g_list_free (data.files);
  return FALSE;
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

