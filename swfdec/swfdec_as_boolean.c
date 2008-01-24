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

#include <math.h>

#include "swfdec_as_boolean.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsBoolean, swfdec_as_boolean, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_boolean_class_init (SwfdecAsBooleanClass *klass)
{
}

static void
swfdec_as_boolean_init (SwfdecAsBoolean *boolean)
{
}

/*** AS CODE ***/

SWFDEC_AS_CONSTRUCTOR (107, 2, swfdec_as_boolean_construct, swfdec_as_boolean_get_type)
void
swfdec_as_boolean_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gboolean b;

  if (argc > 0) {
    b = swfdec_as_value_to_boolean (object->context, &argv[0]);
  } else {
    b = FALSE;
  }

  if (swfdec_as_context_is_constructing (cx)) {
    SWFDEC_AS_BOOLEAN (object)->boolean = b;
    SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, b);
  }
}

SWFDEC_AS_NATIVE (107, 1, swfdec_as_boolean_toString)
void
swfdec_as_boolean_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsBoolean *b;
  
  if (!SWFDEC_IS_AS_BOOLEAN (object))
    return;
  b = SWFDEC_AS_BOOLEAN (object);
  
  SWFDEC_AS_VALUE_SET_STRING (ret, b->boolean ? SWFDEC_AS_STR_true : SWFDEC_AS_STR_false);
}

SWFDEC_AS_NATIVE (107, 0, swfdec_as_boolean_valueOf)
void
swfdec_as_boolean_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsBoolean *b;

  if (!SWFDEC_IS_AS_BOOLEAN (object))
    return;
  b = SWFDEC_AS_BOOLEAN (object);
  
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, b->boolean);
}

// only available as ASnative
SWFDEC_AS_NATIVE (3, 2, swfdec_as_boolean_old_constructor)
void
swfdec_as_boolean_old_constructor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("old 'Boolean' function (only available as ASnative)");
}
