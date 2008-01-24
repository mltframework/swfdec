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

#include <string.h>
#include <unistd.h>

#include "swfdec_test_test.h"
#include "swfdec_test_function.h"
#include "swfdec_test_image.h"

static void
swfdec_test_throw (SwfdecAsContext *cx, const char *message, ...)
{
  SwfdecAsValue val;

  if (!swfdec_as_context_catch (cx, &val)) {
    va_list varargs;
    char *s;

    va_start (varargs, message);
    s = g_strdup_vprintf (message, varargs);
    va_end (varargs);

    /* FIXME: Throw a real object here? */
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, s));
  }
  swfdec_as_context_throw (cx, &val);
}

/*** trace capturing ***/

static char *
swfdec_test_test_trace_diff (SwfdecTestTest *test)
{
  const char *command[] = { "diff", "-u", test->trace_filename, NULL, NULL };
  char *tmp, *diff;
  int fd;
  GSList *walk;

  fd = g_file_open_tmp (NULL, &tmp, NULL);
  if (fd < 0)
    return FALSE;

  test->trace_captured = g_slist_reverse (test->trace_captured);
  for (walk = test->trace_captured; walk; walk = walk->next) {
    const char *s = walk->data;
    ssize_t len = strlen (s);
    if (write (fd, s, len) != len ||
        write (fd, "\n", 1) != 1) {
      close (fd);
      unlink (tmp);
      g_free (tmp);
      return NULL;
    }
  }
  close (fd);
  command[3] = tmp;
  if (!g_spawn_sync (NULL, (char **) command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	&diff, NULL, NULL, NULL)) {
    unlink (tmp);
    g_free (tmp);
    return NULL;
  }
  unlink (tmp);
  g_free (tmp);
  return diff;
}

static void
swfdec_test_test_trace_stop (SwfdecTestTest *test)
{
  if (test->trace_filename == NULL)
    return;

  if (test->trace_buffer &&
      test->trace_offset != test->trace_buffer->data + test->trace_buffer->length)
    test->trace_failed = TRUE;

  if (test->trace_failed) {
    char *diff = swfdec_test_test_trace_diff (test);
    /* FIXME: produce a diff here */
    swfdec_test_throw (SWFDEC_AS_OBJECT (test)->context, "invalid trace output:\n%s", diff);
    g_free (diff);
  }

  if (test->trace_buffer) {
    swfdec_buffer_unref (test->trace_buffer);
    test->trace_buffer = NULL;
  }
  g_free (test->trace_filename);
  test->trace_filename = NULL;
  g_slist_foreach (test->trace_captured, (GFunc) g_free, NULL);
  g_slist_free (test->trace_captured);
  test->trace_captured = NULL;
}

static void
swfdec_test_test_trace_start (SwfdecTestTest *test, const char *filename)
{
  GError *error = NULL;

  g_assert (test->trace_filename == NULL);
  g_assert (test->trace_captured == NULL);

  test->trace_filename = g_strdup (filename);
  test->trace_failed = FALSE;
  test->trace_buffer = swfdec_buffer_new_from_file (filename, &error);
  if (test->trace_buffer == NULL) {
    swfdec_test_throw (SWFDEC_AS_OBJECT (test)->context, "Could not start trace: %s", error->message);
    g_error_free (error);
    return;
  }
  test->trace_offset = test->trace_buffer->data;
}

static void
swfdec_test_test_trace_cb (SwfdecPlayer *player, const char *message, SwfdecTestTest *test)
{
  gsize len;

  if (test->trace_buffer == NULL)
    return;
  
  test->trace_captured = g_slist_prepend (test->trace_captured, g_strdup (message));
  if (test->trace_failed)
    return;

  len = strlen (message);
  if (len + 1 > test->trace_buffer->length - (test->trace_offset -test->trace_buffer->data)) {
    test->trace_failed = TRUE;
    return;
  }
  if (memcmp (message, test->trace_offset, len) != 0) {
    test->trace_failed = TRUE;
    return;
  }
  test->trace_offset += len;
  if (test->trace_offset[0] != '\n') {
    test->trace_failed = TRUE;
    return;
  }
  test->trace_offset++;
}

SWFDEC_TEST_FUNCTION ("Test_trace", swfdec_test_test_trace, 0)
void
swfdec_test_test_trace (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  const char *filename;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "|s", &filename);

  swfdec_test_test_trace_stop (test);
  if (filename[0] == '\0')
    return;
  swfdec_test_test_trace_start (test, filename);
}

/*** SWFDEC_TEST_TEST ***/

G_DEFINE_TYPE (SwfdecTestTest, swfdec_test_test, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_test_dispose (GObject *object)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST (object);

  /* FIXME: this can throw, is that ok? */
  swfdec_test_test_trace_stop (test);

  g_free (test->filename);
  test->filename = NULL;
  if (test->player) {
    g_signal_handlers_disconnect_matched (test, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, test);
    g_object_unref (test->player);
    test->player = NULL;
  }

  G_OBJECT_CLASS (swfdec_test_test_parent_class)->dispose (object);
}

static void
swfdec_test_test_class_init (SwfdecTestTestClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_test_test_dispose;
}

static void
swfdec_test_test_init (SwfdecTestTest *this)
{
}

static void
swfdec_test_test_fscommand (SwfdecPlayer *player, const char *command, 
    const char *para, SwfdecTestTest *test)
{
  if (g_ascii_strcasecmp (command, "quit") == 0) {
    test->player_quit = TRUE;
  }
}

static gboolean
swfdec_test_test_ensure_player (SwfdecTestTest *test)
{
  SwfdecURL *url;

  if (test->filename == NULL)
    return FALSE;
  if (test->player)
    return TRUE;

  g_assert (test->player_quit == FALSE);
  test->player = swfdec_player_new (NULL);
  url = swfdec_url_new_from_input (test->filename);
  swfdec_player_set_url (test->player, url);
  swfdec_url_free (url);
  g_signal_connect (test->player, "fscommand", G_CALLBACK (swfdec_test_test_fscommand), test);
  g_signal_connect (test->player, "trace", G_CALLBACK (swfdec_test_test_trace_cb), test);
  return TRUE;
}

static void
swfdec_test_do_reset (SwfdecTestTest *test, const char *filename)
{
  if (test->player) {
    g_signal_handlers_disconnect_matched (test, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, test);
    g_object_unref (test->player);
    test->player = NULL;
  }
  if (filename == NULL)
    return;

  g_free (test->filename);
  test->filename = g_strdup (filename);
  test->player_quit = FALSE;
}

SWFDEC_TEST_FUNCTION ("Test_advance", swfdec_test_test_advance, 0)
void
swfdec_test_test_advance (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  int msecs;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "i", &msecs);

  if (msecs < 0 || test->player_quit)
    return;
  if (!swfdec_test_test_ensure_player (test))
    return;
  if (msecs == 0) {
    if (!test->player_quit)
      swfdec_player_advance (test->player, 0);
  } else {
    while (msecs > 0 && !test->player_quit) {
      int next_event = swfdec_player_get_next_event (test->player);
      if (next_event < 0)
	break;
      next_event = MIN (next_event, msecs);
      swfdec_player_advance (test->player, next_event);
      msecs -= next_event;
    }
  }
}

SWFDEC_TEST_FUNCTION ("Test_reset", swfdec_test_test_reset, 0)
void
swfdec_test_test_reset (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  const char *filename;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "|s", &filename);

  swfdec_test_do_reset (test, filename);
}

SWFDEC_TEST_FUNCTION ("Test_mouse_move", swfdec_test_test_mouse_move, 0)
void
swfdec_test_test_mouse_move (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  double x, y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "nn", &x, &y);

  if (!swfdec_test_test_ensure_player (test))
    return;

  swfdec_player_mouse_move (test->player, x, y);
}

SWFDEC_TEST_FUNCTION ("Test_mouse_press", swfdec_test_test_mouse_press, 0)
void
swfdec_test_test_mouse_press (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  double x, y;
  int button;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "nn|i", &x, &y, &button);

  if (!swfdec_test_test_ensure_player (test))
    return;

  button = CLAMP (button, 1, 32);
  swfdec_player_mouse_press (test->player, x, y, button);
}

SWFDEC_TEST_FUNCTION ("Test_mouse_release", swfdec_test_test_mouse_release, 0)
void
swfdec_test_test_mouse_release (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  double x, y;
  int button;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "nn|i", &x, &y, &button);

  if (!swfdec_test_test_ensure_player (test))
    return;

  button = CLAMP (button, 1, 32);
  swfdec_player_mouse_release (test->player, x, y, button);
}

SWFDEC_TEST_FUNCTION ("Test_render", swfdec_test_test_render, 0)
void
swfdec_test_test_render (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  SwfdecAsObject *image;
  int x, y, w, h;
  cairo_t *cr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "|iiii", &x, &y, &w, &h);

  if (!swfdec_test_test_ensure_player (test))
    return;

  if (argc == 0) {
    swfdec_player_get_size (test->player, &w, &h);
    if (w < 0 || h < 0)
      swfdec_player_get_default_size (test->player, (guint *) &w, (guint *) &h);
  }
  image = swfdec_test_image_new (cx, w, h);
  if (image == NULL)
    return;
  cr = cairo_create (SWFDEC_TEST_IMAGE (image)->surface);
  cairo_translate (cr, -x, -y);
  swfdec_player_render (test->player, cr, x, y, w, h);
  cairo_destroy (cr);
  SWFDEC_AS_VALUE_SET_OBJECT (retval, image);
}

SWFDEC_TEST_FUNCTION ("Test", swfdec_test_test_new, swfdec_test_test_get_type)
void
swfdec_test_test_new (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  const char *filename;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "|s", &filename);

  swfdec_test_do_reset (test, filename[0] ? filename : NULL);
}

SWFDEC_TEST_FUNCTION ("Test_get_rate", swfdec_test_test_get_rate, 0)
void
swfdec_test_test_get_rate (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "");

  SWFDEC_AS_VALUE_SET_NUMBER (retval, test->player ? swfdec_player_get_rate (test->player) : 0);
}

