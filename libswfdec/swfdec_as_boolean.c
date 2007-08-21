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

static void
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

static void
swfdec_as_boolean_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsBoolean *b = SWFDEC_AS_BOOLEAN (object);
  
  SWFDEC_AS_VALUE_SET_STRING (ret, b->boolean ? SWFDEC_AS_STR_true : SWFDEC_AS_STR_false);
}

static void
swfdec_as_boolean_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsBoolean *b = SWFDEC_AS_BOOLEAN (object);

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, b->boolean);
}

void
swfdec_as_boolean_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *boolean, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  boolean = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_Boolean, SWFDEC_TYPE_AS_BOOLEAN, swfdec_as_boolean_construct, 0));
  if (!boolean)
    return;
  swfdec_as_native_function_set_construct_type (SWFDEC_AS_NATIVE_FUNCTION (boolean), SWFDEC_TYPE_AS_BOOLEAN);
  swfdec_as_native_function_set_object_type (SWFDEC_AS_NATIVE_FUNCTION (boolean), SWFDEC_TYPE_AS_BOOLEAN);
  proto = swfdec_as_object_new (context);
  /* set the right properties on the Boolean object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (boolean, SWFDEC_AS_STR_prototype, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable (boolean, SWFDEC_AS_STR_constructor, &val);
  /* set the right properties on the Boolean.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR___proto__, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, boolean);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, SWFDEC_TYPE_AS_BOOLEAN, swfdec_as_boolean_toString, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_valueOf, SWFDEC_TYPE_AS_BOOLEAN, swfdec_as_boolean_valueOf, 0);
}

