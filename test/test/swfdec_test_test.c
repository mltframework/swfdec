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

#include "swfdec_test_test.h"
#include "swfdec_test_function.h"

/*** trace capturing ***/



/*** SWFDEC_TEST_TEST ***/

G_DEFINE_TYPE (SwfdecTestTest, swfdec_test_test, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_test_dispose (GObject *object)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST (object);

  g_free (test->filename);
  test->filename = NULL;
  if (test->player) {
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

#if 0
static void
swfdec_test_throw (SwfdecAsContext *cx, const char *error)
{
  SwfdecAsValue val;

  if (!swfdec_as_context_catch (cx, &val)) {
    /* FIXME: Throw a real object here? */
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, error));
  }
  swfdec_as_context_throw (cx, &val);
}
#endif

static void
swfdec_test_test_fscommand (SwfdecPlayer *player, const char *command, 
    const char *para, SwfdecTestTest *test)
{
  if (g_ascii_strcasecmp (command, "quit")) {
    test->player_quit = TRUE;
  }
}

static gboolean
swfdec_test_test_ensure_player (SwfdecTestTest *test)
{
  if (test->filename == NULL)
    return FALSE;

  test->player = swfdec_player_new_from_file (test->filename);
  g_signal_connect (test, "fscommand", G_CALLBACK (swfdec_test_test_fscommand), test);
  return TRUE;
}

static void
swfdec_test_do_reset (SwfdecTestTest *test, const char *filename)
{
  if (test->player) {
    g_object_unref (test->player);
    test->player = NULL;
  }
  if (filename == NULL)
    return;

  test->filename = g_strdup (filename);
}

SWFDEC_TEST_FUNCTION ("Test_advance", swfdec_test_test_advance, 0)
void
swfdec_test_test_advance (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  int msecs;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "i", &msecs);

  if (msecs <= 0 || test->player_quit)
    return;
  swfdec_test_test_ensure_player (test);
  while (msecs > 0 && !test->player_quit) {
    int next_event = swfdec_player_get_next_event (test->player);
    if (next_event < 0)
      break;
    next_event = MIN (next_event, msecs);
    swfdec_player_advance (test->player, next_event);
    msecs -= next_event;
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

SWFDEC_TEST_FUNCTION ("Test", swfdec_test_test_new, swfdec_test_test_get_type)
void
swfdec_test_test_new (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  const char *filename;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "|s", &filename);

  swfdec_test_do_reset (test, filename);
}

SWFDEC_TEST_FUNCTION ("Test_rate", swfdec_test_test_get_rate, 0)
void
swfdec_test_test_get_rate (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "");

  SWFDEC_AS_VALUE_SET_NUMBER (retval, test->player ? swfdec_player_get_rate (test->player) : 0);
}

