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
#include "swfdec_test_buffer.h"
#include "swfdec_test_function.h"
#include "swfdec_test_image.h"
#include "swfdec_test_utils.h"

/*** SWFDEC_TEST_TEST ***/

G_DEFINE_TYPE (SwfdecTestTest, swfdec_test_test, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_test_dispose (GObject *object)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST (object);

  if (test->trace) {
    swfdec_buffer_queue_unref (test->trace);
    test->trace = NULL;
  }

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
swfdec_test_test_init (SwfdecTestTest *test)
{
  test->trace = swfdec_buffer_queue_new ();
}

static void
swfdec_test_test_fscommand (SwfdecPlayer *player, const char *command, 
    const char *para, SwfdecTestTest *test)
{
  if (g_ascii_strcasecmp (command, "quit") == 0) {
    test->player_quit = TRUE;
  }
}

static void
swfdec_test_test_trace_cb (SwfdecPlayer *player, const char *message, SwfdecTestTest *test)
{
  gsize len = strlen (message);
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_new_and_alloc (len + 1);
  memcpy (buffer->data, message, len);
  buffer->data[len] = '\n';
  swfdec_buffer_queue_push (test->trace, buffer);
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
  swfdec_buffer_queue_clear (test->trace);
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

SWFDEC_TEST_FUNCTION ("Test_get_trace", swfdec_test_test_get_trace, 0)
void
swfdec_test_test_get_trace (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  SwfdecAsObject *o;
  SwfdecBuffer *buffer;
  gsize len;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "");

  len = swfdec_buffer_queue_get_depth (test->trace);
  buffer = swfdec_buffer_queue_peek (test->trace, len);
  o = swfdec_test_buffer_new (cx, buffer);
  if (o == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (retval, o);
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

