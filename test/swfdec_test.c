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

#include <stdlib.h>
#include <string.h>

#include <swfdec/swfdec.h>
/* FIXME: no internal headers please */
#include <swfdec/swfdec_as_array.h>
#include <swfdec/swfdec_as_internal.h>

#include "swfdec_test_function.h"
#include "swfdec_test_initialize.h"
#include "swfdec_test_test.h"

/*** VERIFICATION OF ENVIRONMENT ***/

static gboolean
check_cairo (gboolean verbose)
{
#define CAIRO_MIN_MAJOR 1
#define CAIRO_MIN_MINOR 7
#define CAIRO_MIN_MICRO 1
#define CAIRO_MAX_MAJOR 1
#define CAIRO_MAX_MINOR 9
#define CAIRO_MAX_MICRO 0
  if (cairo_version () < CAIRO_VERSION_ENCODE (CAIRO_MIN_MAJOR, CAIRO_MIN_MINOR, CAIRO_MIN_MICRO)) {
    g_print ("ERROR: Cairo version %s cannot be used to run tests; must be at least %u.%u.%u.\n",
	cairo_version_string (), CAIRO_MIN_MAJOR, CAIRO_MIN_MINOR, CAIRO_MIN_MICRO);
    return FALSE;
#ifdef CAIRO_MAX_MAJOR
  } else if (cairo_version () >= CAIRO_VERSION_ENCODE (CAIRO_MAX_MAJOR, CAIRO_MAX_MINOR, CAIRO_MAX_MICRO)) {
    g_print ("ERROR: Cairo version %s cannot be used to run tests; must be less than %u.%u.%u.\n",
	cairo_version_string (), CAIRO_MAX_MAJOR, CAIRO_MAX_MINOR, CAIRO_MAX_MICRO);
    return FALSE;
#endif
  } else if (verbose) {
    g_print ("   OK: Cairo version %s is ok; must be at least %u.%u.%u.\n",
	cairo_version_string (), CAIRO_MIN_MAJOR, CAIRO_MIN_MINOR, CAIRO_MIN_MICRO);
  }
  return TRUE;
}

/* FIXME */
#include <swfdec/swfdec_audio_decoder.h>
static gboolean
check_codecs (gboolean verbose)
{
  if (!swfdec_audio_decoder_prepare (SWFDEC_AUDIO_CODEC_MP3,
	swfdec_audio_format_new (44100, 2, TRUE), NULL)) {
    g_print ("ERROR: MP3 support is not available.\n");
    return FALSE;
  } else if (verbose) {
    g_print ("   OK: MP3 support is available.\n");
  }
  return TRUE;
}

static gboolean
check_system (gboolean verbose)
{
  gboolean ret = TRUE;

  /* We want to run all checks, so don't use && here */
  ret &= check_cairo (verbose);
  ret &= check_codecs (verbose);

  return ret;
}


/* Start of script file */
#define SWFDEC_TEST_FILE_ID "Swfdec Test Script\0\1"
/* Flash version the script engine runs in */
#define SWFDEC_TEST_VERSION 8

static SwfdecScript *
load_script (const char *filename)
{
  SwfdecBuffer *file, *buffer;
  SwfdecScript *script;
  GError *error = NULL;

  if (filename == NULL)
    filename = "default.sts";

  file = swfdec_buffer_new_from_file (filename, &error);
  if (file == NULL) {
    g_print ("ERROR: %s\n", error->message);
    g_error_free (error);
    return NULL;
  }
  if (file->length < sizeof (SWFDEC_TEST_FILE_ID) + 1 ||
      memcmp (file->data, SWFDEC_TEST_FILE_ID, sizeof (SWFDEC_TEST_FILE_ID) != 0)) {
    g_print ("ERROR: %s is not a Swfdec test script\n", filename);
    swfdec_buffer_unref (file);
    return NULL;
  }
  buffer = swfdec_buffer_new_subbuffer (file, sizeof (SWFDEC_TEST_FILE_ID), 
      file->length - sizeof (SWFDEC_TEST_FILE_ID));
  swfdec_buffer_unref (file);
  script = swfdec_script_new (buffer, "main", SWFDEC_TEST_VERSION);
  return script;
}

int
main (int argc, char **argv)
{
  char *script_filename = NULL;
  GError *error = NULL;
  SwfdecAsContext *context;
  SwfdecAsObject *array;
  SwfdecScript *script;
  SwfdecAsValue val;
  int i, ret;
  gboolean dump = FALSE;
  gboolean no_check = FALSE, only_check = FALSE;

  GOptionEntry options[] = {
    { "dump", 'd', 0, G_OPTION_ARG_NONE, &dump, "dump informative output on failure", FALSE },
    { "no-check", 0, 0, G_OPTION_ARG_NONE, &no_check, "don't check if the system is ok for running the testsuite", FALSE },
    { "self-check", 0, 0, G_OPTION_ARG_NONE, &only_check, "run a system check and exit", FALSE },
    { "player", 'p', 0, G_OPTION_ARG_STRING, &swfdec_test_plugin_name, "player to test", "NAME" },
    { "script", 's', 0, G_OPTION_ARG_STRING, &script_filename, "script to execute if not ./default.sts", "FILENAME" },
    { NULL }
  };
  GOptionContext *ctx;

  /* set the right warning levels */
  g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
  /* by default get rid of the loads of warnings the tests produce */
  g_setenv ("SWFDEC_DEBUG", "2", FALSE);

  g_thread_init (NULL);
  swfdec_init ();

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);

  if (error) {
    g_printerr ("ERROR: wrong command line arguments: %s\n", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  if (only_check || !no_check) {
    gboolean result = check_system (only_check);
    if (!result) {
      g_print ("ERROR: System checked failed, aborting. Use --no-check to disable.\n");
      return 1;
    } else if (only_check) {
      return 0;
    }
  }
  g_assert (!only_check);

  /* allow env vars instead of options - eases running make check with different settings */
  if (swfdec_test_plugin_name == NULL)
    swfdec_test_plugin_name = g_strdup (g_getenv ("SWFDEC_TEST_PLAYER"));

  script = load_script (script_filename);
  g_free (script_filename);
  if (script == NULL)
    return EXIT_FAILURE;

  context = g_object_new (SWFDEC_TYPE_AS_CONTEXT, NULL);
  swfdec_as_context_startup (context);

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, dump);
  swfdec_as_object_set_variable (context->global,
      swfdec_as_context_get_string (context, "dump"), &val);

  swfdec_test_function_init_context (context);
  swfdec_as_context_run_init_script (context, swfdec_test_initialize, 
      sizeof (swfdec_test_initialize), SWFDEC_TEST_VERSION);

  array = swfdec_as_array_new (context);
  if (array == NULL) {
    g_print ("ERROR: Not enough memory");
    return EXIT_FAILURE;
  }
  if (argc < 2) {
    GDir *dir;
    const char *file;
    dir = g_dir_open (".", 0, NULL);
    while ((file = g_dir_read_name (dir))) {
      if (!g_str_has_suffix (file, ".swf"))
	continue;
      SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (context, file));
      swfdec_as_array_push (array, &val);
    }
    g_dir_close (dir);
  } else {
    for (i = 1; i < argc; i++) {
      SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (context, argv[i]));
      swfdec_as_array_push (array, &val);
    }
  }
  SWFDEC_AS_VALUE_SET_OBJECT (&val, array);
  swfdec_as_object_set_variable (context->global, 
    swfdec_as_context_get_string (context, "filenames"), &val);
  swfdec_as_object_run (context->global, script);
  if (swfdec_as_context_catch (context, &val)) {
    g_print ("ERROR: %s\n", swfdec_as_value_to_string (context, val));
    ret = EXIT_FAILURE;
  } else {
    g_print ("SUCCESS\n");
    ret = EXIT_SUCCESS;
  }

  swfdec_script_unref (script);
  g_object_unref (context);

  return ret;
}

