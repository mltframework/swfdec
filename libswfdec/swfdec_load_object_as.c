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
  SwfdecAsValue val;

  SWFDEC_AS_VALUE_SET_NUMBER (&val, loaded);
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR__bytesLoaded,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);

  if (size >= 0) {
    SWFDEC_AS_VALUE_SET_NUMBER (&val, size);
  } else {
    SWFDEC_AS_VALUE_SET_NUMBER (&val, loaded);
  }
  swfdec_as_object_set_variable_and_flags (target, SWFDEC_AS_STR__bytesTotal,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);
}

SWFDEC_AS_NATIVE (301, 0, swfdec_load_object_as_load)
void
swfdec_load_object_as_load (SwfdecAsContext *cx, SwfdecAsObject *obj, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsValue val;
  const char *url;

  if (argc < 1 || obj == NULL) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    return;
  }

  url = swfdec_as_value_to_string (cx, &argv[0]);
  swfdec_load_object_new (obj, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL,
      swfdec_load_object_on_progress, swfdec_load_object_on_finish);

  SWFDEC_AS_VALUE_SET_INT (&val, 0);
  swfdec_as_object_set_variable_and_flags (obj, SWFDEC_AS_STR__bytesLoaded,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);
  SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  swfdec_as_object_set_variable_and_flags (obj, SWFDEC_AS_STR__bytesTotal,
      &val, SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, FALSE);
  swfdec_as_object_set_variable_and_flags (obj, SWFDEC_AS_STR_loaded, &val,
      SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

SWFDEC_AS_NATIVE (301, 1, swfdec_load_object_as_send)
void
swfdec_load_object_as_send (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("LoadVars/XML.send");
}

SWFDEC_AS_NATIVE (301, 2, swfdec_load_object_as_sendAndLoad)
void
swfdec_load_object_as_sendAndLoad (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *url, *data;
  SwfdecAsObject *target;
  SwfdecAsValue val;
  SwfdecBuffer *buffer;

  if (object == NULL)
    return;

  if (argc < 2)
    return;

  url = swfdec_as_value_to_string (cx, &argv[0]);
  target = swfdec_as_value_to_object (cx, &argv[0]);
  if (target == NULL)
    return;

  // FIXME: support for contentType is missing

  swfdec_as_object_call (object, SWFDEC_AS_STR_toString, 0, NULL, &val);
  data = swfdec_as_value_to_string (cx, &val);
  if (strlen (data) > 0) {
    buffer = swfdec_buffer_new_for_data (g_memdup (data, strlen (data)),
	strlen (data));
    swfdec_load_object_new (target, url, SWFDEC_LOADER_REQUEST_POST, buffer,
	swfdec_load_object_on_progress, swfdec_load_object_on_finish);
    swfdec_buffer_unref (buffer);
  } else {
    swfdec_load_object_new (target, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL,
	swfdec_load_object_on_progress, swfdec_load_object_on_finish);
  }

  SWFDEC_AS_VALUE_SET_INT (&val, 0);
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
