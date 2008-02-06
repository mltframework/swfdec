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
#include "swfdec_test_socket.h"
#include "swfdec_test_utils.h"

/*** PLUGIN HANDLING ***/

#define SWFDEC_TEST_TEST_FROM_PLUGIN(x) \
  SWFDEC_TEST_TEST ((gpointer) ((guint8 *) (x) - G_STRUCT_OFFSET (SwfdecTestTest, plugin)))

char *swfdec_test_plugin_name = NULL;

static void
swfdec_test_test_quit (SwfdecTestPlugin *plugin)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST_FROM_PLUGIN (plugin);

  test->plugin_quit = TRUE;
}

static void
swfdec_test_test_error (SwfdecTestPlugin *plugin, const char *description)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST_FROM_PLUGIN (plugin);

  if (test->plugin_error)
    return;
  test->plugin_error = TRUE;
  swfdec_test_throw (SWFDEC_AS_OBJECT (test)->context, description);
}

static void
swfdec_test_test_trace (SwfdecTestPlugin *plugin, const char *message)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST_FROM_PLUGIN (plugin);
  gsize len = strlen (message);
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_new (len + 1);
  memcpy (buffer->data, message, len);
  buffer->data[len] = '\n';
  swfdec_buffer_queue_push (test->trace, buffer);
}

static void
swfdec_test_test_launch (SwfdecTestPlugin *plugin, const char *url)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST_FROM_PLUGIN (plugin);
  gsize len = strlen (url);
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_new (len + 1);
  memcpy (buffer->data, url, len);
  buffer->data[len] = '\n';
  swfdec_buffer_queue_push (test->launched, buffer);
}

static void
swfdec_test_test_request_socket (SwfdecTestPlugin *plugin,
    SwfdecTestPluginSocket *psock)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST_FROM_PLUGIN (plugin);

  swfdec_test_socket_new (test, psock);
}

static void
swfdec_test_test_load_plugin (SwfdecTestTest *test, const char *filename)
{
  memset (&test->plugin, 0, sizeof (SwfdecTestPlugin));
  /* initialize test->plugin */
  /* FIXME: This assumes filenames - do we wanna allow http? */
  if (g_path_is_absolute (filename)) {
    test->plugin.filename = g_strdup (filename);
  } else {
    char *cur = g_get_current_dir ();
    test->plugin.filename = g_build_filename (cur, filename, NULL);
    g_free (cur);
  }
  test->plugin.trace = swfdec_test_test_trace;
  test->plugin.launch = swfdec_test_test_launch;
  test->plugin.quit = swfdec_test_test_quit;
  test->plugin.error = swfdec_test_test_error;
  test->plugin.request_socket = swfdec_test_test_request_socket;

  /* load the right values */
  if (swfdec_test_plugin_name) {
    void (*init) (SwfdecTestPlugin *plugin);
    char *dir = g_build_filename (g_get_home_dir (), ".swfdec-test", NULL);
    char *name = g_module_build_path (dir, swfdec_test_plugin_name);
    g_free (dir);
    test->module = g_module_open (name, G_MODULE_BIND_LOCAL);
    if (test->module == NULL) {
      swfdec_test_throw (SWFDEC_AS_OBJECT (test)->context, "could not find player \"%s\"",
	  swfdec_test_plugin_name);
      return;
    }
    if (!g_module_symbol (test->module, "swfdec_test_plugin_init", (gpointer) &init)) {
      g_module_close (test->module);
      test->module = NULL;
    }
    init (&test->plugin);
  } else {
    swfdec_test_plugin_swfdec_new (&test->plugin);
  }
  test->plugin_loaded = TRUE;
}

static void
swfdec_test_test_unload_plugin (SwfdecTestTest *test)
{
  if (!test->plugin_loaded)
    return;
  test->plugin.finish (&test->plugin);
  g_free (test->plugin.filename);
  if (test->module) {
    g_module_close (test->module);
    test->module = NULL;
  }
  test->plugin_quit = FALSE;
  test->plugin_error = FALSE;
  test->plugin_loaded = FALSE;
}

/*** SWFDEC_TEST_TEST ***/

G_DEFINE_TYPE (SwfdecTestTest, swfdec_test_test, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_test_dispose (GObject *object)
{
  SwfdecTestTest *test = SWFDEC_TEST_TEST (object);

  while (test->sockets)
    swfdec_test_socket_close (test->sockets->data);
  test->pending_sockets = NULL;

  test->plugin_error = TRUE; /* set to avoid callbacks into the engine */
  swfdec_test_test_unload_plugin (test);

  if (test->trace) {
    swfdec_buffer_queue_unref (test->trace);
    test->trace = NULL;
  }

  g_free (test->filename);
  test->filename = NULL;

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
  test->launched = swfdec_buffer_queue_new ();
}

static void
swfdec_test_do_reset (SwfdecTestTest *test, const char *filename)
{
  swfdec_test_test_unload_plugin (test);
  swfdec_buffer_queue_clear (test->trace);
  if (filename == NULL)
    return;

  swfdec_test_test_load_plugin (test, filename);
}

SWFDEC_TEST_FUNCTION ("Test_advance", swfdec_test_test_advance, 0)
void
swfdec_test_test_advance (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  int msecs;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "i", &msecs);

  if (msecs < 0 || !test->plugin_loaded || test->plugin_error || test->plugin_quit)
    return;

  if (test->plugin.advance) {
    test->plugin.advance (&test->plugin, msecs);
  } else {
    swfdec_test_throw (cx, "plugin doesn't implement advance");
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

  if (!test->plugin_loaded || test->plugin_error || test->plugin_quit)
    return;

  if (test->plugin.advance) {
    test->plugin.mouse_move (&test->plugin, x, y);
  } else {
    swfdec_test_throw (cx, "plugin doesn't implement mouse_move");
  }
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

  if (!test->plugin_loaded || test->plugin_error || test->plugin_quit)
    return;

  button = CLAMP (button, 1, 32);
  if (test->plugin.advance) {
    test->plugin.mouse_press (&test->plugin, x, y, button);
  } else {
    swfdec_test_throw (cx, "plugin doesn't implement mouse_press");
  }
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

  if (!test->plugin_loaded || test->plugin_error || test->plugin_quit)
    return;

  button = CLAMP (button, 1, 32);
  if (test->plugin.advance) {
    test->plugin.mouse_release (&test->plugin, x, y, button);
  } else {
    swfdec_test_throw (cx, "plugin doesn't implement mouse_press");
  }
}

SWFDEC_TEST_FUNCTION ("Test_render", swfdec_test_test_render, 0)
void
swfdec_test_test_render (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  SwfdecAsObject *image;
  int x, y, w, h;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "|iiii", &x, &y, &w, &h);

  if (!test->plugin_loaded || test->plugin_error || test->plugin_quit)
    return;

  if (argc == 0) {
    w = test->plugin.width;
    h = test->plugin.height;
  }
  image = swfdec_test_image_new (cx, w, h);
  if (image == NULL)
    return;

  if (test->plugin.screenshot) {
    test->plugin.screenshot (&test->plugin, 
	cairo_image_surface_get_data (SWFDEC_TEST_IMAGE (image)->surface),
	x, y, w, h);
    SWFDEC_AS_VALUE_SET_OBJECT (retval, image);
  } else {
    swfdec_test_throw (cx, "plugin doesn't implement mouse_press");
  }
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

SWFDEC_TEST_FUNCTION ("Test_get_launched", swfdec_test_test_get_launched, 0)
void
swfdec_test_test_get_launched (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;
  SwfdecAsObject *o;
  SwfdecBuffer *buffer;
  gsize len;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "");

  len = swfdec_buffer_queue_get_depth (test->launched);
  buffer = swfdec_buffer_queue_peek (test->launched, len);
  o = swfdec_test_buffer_new (cx, buffer);
  if (o == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (retval, o);
}

SWFDEC_TEST_FUNCTION ("Socket_getSocket", swfdec_test_test_getSocket, 0)
void
swfdec_test_test_getSocket (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestTest *test;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_TEST, &test, "");

  if (test->pending_sockets == NULL) {
    if (test->sockets == NULL) {
      SWFDEC_AS_VALUE_SET_NULL (retval);
    } else {
      test->pending_sockets = test->sockets;
      SWFDEC_AS_VALUE_SET_OBJECT (retval, test->pending_sockets->data);
    }
  } else {
    if (test->pending_sockets->next == NULL) {
      SWFDEC_AS_VALUE_SET_NULL (retval);
    } else {
      test->pending_sockets = test->pending_sockets->next;
      SWFDEC_AS_VALUE_SET_OBJECT (retval, test->pending_sockets->data);
    }
  }
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

  if (!test->plugin_loaded)
    return;
  
  SWFDEC_AS_VALUE_SET_NUMBER (retval, test->plugin.rate / 256.0);
}

