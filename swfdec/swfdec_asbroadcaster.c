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

#include "swfdec_as_internal.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_debug.h"

/*** AS CODE ***/

SWFDEC_AS_NATIVE (101, 12, broadcastMessage)
void
broadcastMessage (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsValue val;
  SwfdecAsObject *listeners, *o;
  gint i, length;
  const char *name;
  GSList *list = NULL, *walk;

  if (object == NULL)
    return;

  if (argc < 1)
    return;
  name = swfdec_as_value_to_string (cx, argv[0]);
  argv += 1;
  argc--;

  swfdec_as_object_get_variable (object, SWFDEC_AS_STR__listeners, &val);
  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (val))
    return;

  listeners = SWFDEC_AS_VALUE_GET_COMPOSITE (val);
  swfdec_as_object_get_variable (listeners, SWFDEC_AS_STR_length, &val);
  length = swfdec_as_value_to_integer (cx, val);

  /* return undefined if we won't try to call anything */
  if (length <= 0)
    return;

  /* FIXME: solve this wth foreach, so it gets faster for weird cases */
  for (i = 0; i < length; i++) {
    swfdec_as_object_get_variable (listeners, swfdec_as_integer_to_string (cx, i), &val);
    o = swfdec_as_value_to_object (cx, val);
    if (o == NULL)
      continue;
    list = g_slist_prepend (list, o);
  }
  if (list == NULL)
    return;

  list = g_slist_reverse (list);
  for (walk = list; walk; walk = walk->next) {
    swfdec_as_object_call (walk->data, name, argc, argv, &val);
  }
  g_slist_free (list);

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
}

