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

#include <string.h>
#include <unistd.h>

#include "swfdec_test_buffer.h"
#include "swfdec_test_function.h"
#include "swfdec_test_utils.h"

SwfdecTestBuffer *
swfdec_test_buffer_new (SwfdecAsContext *context, SwfdecBuffer *buffer)
{
  SwfdecTestBuffer *ret;
  SwfdecAsObject *object;

  ret = g_object_new (SWFDEC_TYPE_TEST_BUFFER, "context", context, NULL);
  ret->buffer = buffer;

  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_constructor_by_name (object,
    swfdec_as_context_get_string (context, "Buffer"), NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (ret));

  return ret;
}

/*** SWFDEC_TEST_BUFFER ***/

G_DEFINE_TYPE (SwfdecTestBuffer, swfdec_test_buffer, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_test_buffer_dispose (GObject *object)
{
  SwfdecTestBuffer *buffer = SWFDEC_TEST_BUFFER (object);

  if (buffer->buffer) {
    swfdec_buffer_unref (buffer->buffer);
    buffer->buffer = NULL;
  }

  G_OBJECT_CLASS (swfdec_test_buffer_parent_class)->dispose (object);
}

static void
swfdec_test_buffer_class_init (SwfdecTestBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_test_buffer_dispose;
}

static void
swfdec_test_buffer_init (SwfdecTestBuffer *buffer)
{
}

/*** AS CODE ***/

static char *
swfdec_test_diff_buffers (SwfdecBuffer *buf1, SwfdecBuffer *buf2, GError **error)
{
  const char *command[] = { "diff", "-u", NULL, NULL, NULL };
  char *file1, *file2, *diff;
  int fd;

  /* shortcut the (hopefully) common case of equality */
  if (buf1->length == buf2->length && 
      memcmp (buf1->data, buf2->data, buf1->length) == 0)
    return NULL;

  /* write the first buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &file1, error);
  if (fd < 0)
    return NULL;
  if (write (fd, buf1->data, buf1->length) != (int) buf1->length) {
    close (fd);
    g_free (file1);
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	"Could not write data to temporary file");
    return NULL;
  }
  close (fd);

  /* write the second buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &file2, error);
  if (fd < 0) {
    g_free (file1);
    return NULL;
  }
  if (write (fd, buf2->data, buf2->length) != (int) buf2->length) {
    close (fd);
    g_free (file1);
    g_free (file2);
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	"Could not write data to temporary file");
    return NULL;
  }
  close (fd);

  /* run diff command */
  command[2] = file1;
  command[3] = file2;
  if (!g_spawn_sync (NULL, (char **) command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
	&diff, NULL, NULL, error)) {
    unlink (file1);
    unlink (file2);
    g_free (file1);
    g_free (file2);
    return NULL;
  }
  unlink (file1);
  unlink (file2);
  g_free (file1);
  g_free (file2);
  return diff;
}

SWFDEC_TEST_FUNCTION ("Buffer_diff", swfdec_test_buffer_diff)
void
swfdec_test_buffer_diff (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestBuffer *buffer, *compare = NULL;
  SwfdecAsObject *compare_object;
  GError *error = NULL;
  char *ret;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_BUFFER, &buffer, "|o", &compare_object);

  if (compare_object == NULL || !SWFDEC_IS_TEST_BUFFER (compare_object->relay)) {
    swfdec_test_throw (cx, "must pass a buffer to Buffer.diff");
    return;
  }
  compare = SWFDEC_TEST_BUFFER (compare_object->relay);

  ret = swfdec_test_diff_buffers (compare->buffer, buffer->buffer, &error);
  if (ret) {
    SWFDEC_AS_VALUE_SET_STRING (retval, 
	swfdec_as_context_give_string (cx, ret));
  } else {
    if (error) {
      swfdec_test_throw (cx, "s", error->message);
      g_error_free (error);
    } else {
      SWFDEC_AS_VALUE_SET_STRING (retval, 
	  swfdec_as_context_get_string (cx, ""));
    }
  }
}

SWFDEC_TEST_FUNCTION ("Buffer_find", swfdec_test_buffer_find)
void
swfdec_test_buffer_find (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  int c;
  SwfdecTestBuffer *buffer;
  guchar *found;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_BUFFER, &buffer, "i", &c);

  swfdec_as_value_set_integer (cx, retval, -1);

  found = memchr (buffer->buffer->data, c, buffer->buffer->length);
  if (found)
    swfdec_as_value_set_integer (cx, retval, found - buffer->buffer->data);
}

SWFDEC_TEST_FUNCTION ("Buffer_load", swfdec_test_buffer_load)
void
swfdec_test_buffer_load (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecBuffer *b;
  SwfdecTestBuffer *buffer;
  const char *filename;
  GError *error = NULL;
  
  SWFDEC_AS_CHECK (0, NULL, "s", &filename);

  b = swfdec_buffer_new_from_file (filename, &error);
  if (b == NULL) {
    swfdec_test_throw (cx, "%s", error->message);
    g_error_free (error);
    return;
  }

  buffer = swfdec_test_buffer_new (cx, b);
  SWFDEC_AS_VALUE_SET_OBJECT (retval, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (buffer)));
}

SWFDEC_TEST_FUNCTION ("Buffer_sub", swfdec_test_buffer_sub)
void
swfdec_test_buffer_sub (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecBuffer *b;
  SwfdecTestBuffer *buffer, *sub;
  guint offset, length = 0;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_BUFFER, &buffer, "i|i", &offset, &length);

  SWFDEC_AS_VALUE_SET_NULL (retval);
  if (offset >= buffer->buffer->length)
    return;

  if (length == 0)
    length = buffer->buffer->length - offset;

  b = swfdec_buffer_new_subbuffer (buffer->buffer, offset, length);
  sub = swfdec_test_buffer_new (cx, b);
  SWFDEC_AS_VALUE_SET_OBJECT (retval, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (sub)));
}

SWFDEC_TEST_FUNCTION ("Buffer_toString", swfdec_test_buffer_toString)
void
swfdec_test_buffer_toString (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecBuffer *b;
  SwfdecTestBuffer *buffer;
  const char *end;
  char *s;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_BUFFER, &buffer, "");

  b = buffer->buffer;
  if (g_utf8_validate ((const char *) b->data, b->length, &end)) {
    s = g_strndup ((const char *) b->data, b->length);
  } else if ((size_t) (end - (char *) b->data) == b->length - 1) {
    s = g_strdup ((const char *) b->data);
  } else {
    s = NULL;
  }
  if (s == NULL)
    s = g_strdup ("[binary Buffer]");
  SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_give_string (cx, s));
}

SwfdecBuffer *
swfdec_test_buffer_from_args (SwfdecAsContext *cx, guint argc, SwfdecAsValue *argv)
{
  SwfdecBufferQueue *queue;
  SwfdecBuffer *buffer;
  guint i;

  queue = swfdec_buffer_queue_new ();
  for (i = 0; i < argc; i++) {
    SwfdecBuffer *b = NULL;
    if (SWFDEC_AS_VALUE_IS_OBJECT (argv[i])) {
      SwfdecAsObject *o = SWFDEC_AS_VALUE_GET_OBJECT (argv[i]);
      if (SWFDEC_IS_TEST_BUFFER (o->relay))
	b = swfdec_buffer_ref (SWFDEC_TEST_BUFFER (o->relay)->buffer);
    } else if (SWFDEC_AS_VALUE_IS_NUMBER (argv[i])) {
      b = swfdec_buffer_new (1);
      b->data[0] = swfdec_as_value_to_integer (cx, &argv[i]);
    }
    if (b == NULL) {
      const char *s = swfdec_as_value_to_string (cx, argv[i]);
      gsize len = strlen (s);
      /* no terminating 0 byte on purpose here - use new Buffer (string, 0); to get that */
      b = swfdec_buffer_new (len);
      memcpy (b->data, s, len);
    }
    swfdec_buffer_queue_push (queue, b);
  }
  i = swfdec_buffer_queue_get_depth (queue);
  buffer = swfdec_buffer_queue_pull (queue, i);
  swfdec_buffer_queue_unref (queue);

  return buffer;
}

SWFDEC_TEST_FUNCTION ("Buffer", swfdec_test_buffer_create)
void
swfdec_test_buffer_create (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestBuffer *buffer;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  buffer = g_object_new (SWFDEC_TYPE_TEST_BUFFER, "context", cx, NULL);
  buffer->buffer = swfdec_test_buffer_from_args (cx, argc, argv);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (buffer));

  SWFDEC_AS_VALUE_SET_OBJECT (retval, object);
}


