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
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"

SWFDEC_AS_NATIVE (301, 0, swfdec_load_object_as_load)
void
swfdec_load_object_as_load (SwfdecAsContext *cx, SwfdecAsObject *obj, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *url;

  if (argc < 1 || obj == NULL) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    return;
  }

  url = swfdec_as_value_to_string (cx, &argv[0]);
  swfdec_load_object_new (obj, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

SWFDEC_AS_NATIVE (301, 2, swfdec_load_object_as_sendAndLoad)
void
swfdec_load_object_as_sendAndLoad (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *url, *data;
  SwfdecAsObject *target;
  SwfdecAsValue ret;
  SwfdecBuffer *buffer;

  if (object == NULL)
    return;

  if (argc < 2)
    return;

  url = swfdec_as_value_to_string (cx, &argv[0]);
  target = swfdec_as_value_to_object (cx, &argv[0]);
  if (target == NULL)
    return;

  swfdec_as_object_call (object, SWFDEC_AS_STR_toString, 0, NULL, &ret);
  data = swfdec_as_value_to_string (cx, &ret);
  if (strlen (data) > 0) {
    buffer = swfdec_buffer_new_for_data (g_memdup (data, strlen (data)),
	strlen (data));
    swfdec_load_object_new (target, url, SWFDEC_LOADER_REQUEST_POST, buffer);
    swfdec_buffer_unref (buffer);
  } else {
    swfdec_load_object_new (target, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL);
  }

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}
