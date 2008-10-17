/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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
#include "swfdec_load_object.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"

static void
swfdec_load_object_on_finish (SwfdecAsObject *target, const char *text)
{
  SwfdecAsValue val;

  if (text != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (&val, text);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  }

  swfdec_as_object_call (target, SWFDEC_AS_STR_onData, 1, &val, NULL);
}

static void
swfdec_load_object_on_progress (SwfdecAsObject *target, glong size,
    glong loaded)
{
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  cx = swfdec_gc_object_get_context (target);
  swfdec_as_value_set_number (cx, &val, loaded);
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR__bytesLoaded,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);

  if (size >= 0) {
    swfdec_as_value_set_number (cx, &val, size);
  } else {
    swfdec_as_value_set_number (cx, &val, loaded);
  }
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR__bytesTotal,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);
}

SWFDEC_AS_NATIVE (301, 0, swfdec_load_object_as_load)
void
swfdec_load_object_as_load (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsValue val;
  const char *url;

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_OBJECT, &object, "s", &url);

  swfdec_load_object_create (object, url, NULL, 0, NULL, NULL,
      swfdec_load_object_on_progress, swfdec_load_object_on_finish);

  swfdec_as_value_set_integer (cx, &val, 0);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR__bytesLoaded,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);
  SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR__bytesTotal,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, FALSE);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_loaded, &val,
      SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

#define ALLOWED_CHARACTERS " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
static void
swfdec_load_object_as_get_headers (SwfdecAsObject *object, guint *header_count,
    char ***header_names, char ***header_values)
{
  // Should these be filtered at some other point instead?
  static const char *disallowed_names[] = { "Accept-Ranges", "Age", "Allow",
    "Allowed", "Connection", "Content-Length", "Content-Location",
    "Content-Range", "ETag", "Host", "Last-Modified", "Location",
    "Max-Forwards", "Proxy-Authenticate", "Proxy-Authorization", "Public",
    "Range", "Referer", "Retry-After", "Server", "TE", "Trailer",
    "Transfer-Encoding", "Upgrade", "URI", "Vary", "Via", "Warning",
    "WWW-Authenticate", "x-flash-version" };
  GPtrArray *array_names, *array_values;
  SwfdecAsValue val;
  SwfdecAsObject *list;
  int i, length;
  guint j;
  const char *name;
  SwfdecAsContext *cx;

  cx = swfdec_gc_object_get_context (object);

  array_names = g_ptr_array_new ();
  array_values = g_ptr_array_new ();

  if (swfdec_as_object_get_variable (object, SWFDEC_AS_STR_contentType, &val))
  {
    g_ptr_array_add (array_names, g_strdup (SWFDEC_AS_STR_Content_Type));
    g_ptr_array_add (array_values,
	g_strdup (swfdec_as_value_to_string (cx, &val)));
  }

  if (!swfdec_as_object_get_variable (object, SWFDEC_AS_STR__customHeaders,
	&val))
    goto end;
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    SWFDEC_WARNING ("_customHeaders is not an object");
    goto end;
  }
  list = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (list, SWFDEC_AS_STR_length, &val);
  length = swfdec_as_value_to_integer (cx, &val);

  /* FIXME: solve this with foreach, so it gets faster for weird cases */
  name = NULL;
  for (i = 0; i < length; i++) {
    swfdec_as_object_get_variable (list, swfdec_as_integer_to_string (cx, i),
	&val);
    if (name == NULL) {
      name = swfdec_as_value_to_string (cx, &val);
    } else {
      const char *value = swfdec_as_value_to_string (cx, &val);
      for (j = 0; j < G_N_ELEMENTS (disallowed_names); j++) {
	if (g_ascii_strcasecmp (name, disallowed_names[j]) == 0)
	  break;
      }
      if (j < G_N_ELEMENTS (disallowed_names)) {
	SWFDEC_WARNING ("Custom header with name %s is not allowed", name);
      } else if (strspn (name, ALLOWED_CHARACTERS) != strlen (name) || strchr (name, ':') != NULL || strchr (name, ' ') != NULL) {
	SWFDEC_WARNING ("Custom header's name (%s) contains characters that are not allowed", name);
      } else if (strspn (value, ALLOWED_CHARACTERS) != strlen (value)) {
	SWFDEC_WARNING ("Custom header's value (%s) contains characters that are not allowed", value);
      } else {
	g_ptr_array_add (array_names, g_strdup (name));
	g_ptr_array_add (array_values, g_strdup (value));
      }
      name = NULL;
    }
  }
  // if there is uneven amount of elements, just ignore the last one
  if (name != NULL)
    SWFDEC_WARNING ("_customHeaders with uneven amount of elements");

end:
  g_assert (array_names->len == array_values->len);
  *header_count = array_names->len;
  g_ptr_array_add (array_names, NULL);
  g_ptr_array_add (array_values, NULL);
  *header_names = (char **)g_ptr_array_free (array_names, FALSE);
  *header_values = (char **)g_ptr_array_free (array_values, FALSE);
}
#undef ALLOWED_CHARACTERS

SWFDEC_AS_NATIVE (301, 1, swfdec_load_object_as_send)
void
swfdec_load_object_as_send (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *url, *target = SWFDEC_AS_STR_EMPTY, *method = NULL, *data;
  guint header_count;
  char **header_names, **header_values;
  SwfdecAsValue val;
  SwfdecBuffer *buffer;

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_OBJECT, &object, "s|ss", &url, &target, &method);

  SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  data = swfdec_as_value_to_string (cx, &val);

  if (method == NULL || g_ascii_strcasecmp (method, "GET") == 0) {
    url = swfdec_as_context_give_string (cx,
	g_strjoin (NULL, url, "?", data, NULL));
    buffer = NULL;
  } else {
    // don't send the nul-byte
    buffer = swfdec_buffer_new_for_data (g_memdup (data, strlen (data)),
	strlen (data));
  }

  swfdec_load_object_as_get_headers (object, &header_count, &header_names,
      &header_values);
  swfdec_player_launch_with_headers (SWFDEC_PLAYER (cx), url, target, buffer,
      header_count, header_names, header_values);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

SWFDEC_AS_NATIVE (301, 2, swfdec_load_object_as_sendAndLoad)
void
swfdec_load_object_as_sendAndLoad (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *url, *data, *method = NULL;
  guint header_count;
  char **header_names, **header_values;
  SwfdecAsObject *target;
  SwfdecAsValue val;
  SwfdecBuffer *buffer;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_OBJECT, &object, "sO|s", &url, &target,
      &method);

  SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  data = swfdec_as_value_to_string (cx, &val);

  if (method != NULL && g_ascii_strcasecmp (method, "GET") == 0) {
    url = swfdec_as_context_give_string (cx,
	g_strjoin (NULL, url, "?", data, NULL));
    buffer = NULL;
  } else {
    // don't send the nul-byte
    buffer = swfdec_buffer_new_for_data (g_memdup (data, strlen (data)),
	strlen (data));
  }

  swfdec_load_object_as_get_headers (object, &header_count, &header_names,
      &header_values);
  swfdec_load_object_create (target, url, buffer, header_count, header_names,
      header_values, swfdec_load_object_on_progress,
      swfdec_load_object_on_finish);

  swfdec_as_value_set_integer (cx, &val, 0);
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR__bytesLoaded,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);
  SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR__bytesTotal,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, FALSE);
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR_loaded, &val,
      SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}
