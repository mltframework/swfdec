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

#include "swfdec_as_number.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_native_function.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsNumber, swfdec_as_number, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_number_class_init (SwfdecAsNumberClass *klass)
{
}

static void
swfdec_as_number_init (SwfdecAsNumber *number)
{
}

/**
 * swfdec_as_number_new:
 * @context: a #SwfdecAsContext
 * @number: value of the number
 *
 * Creates a new #SwfdecAsNumber. This is the same as executing the Actionscript
 * code "new Number ()"
 *
 * Returns: the new number or %NULL on OOM.
 **/
SwfdecAsObject *
swfdec_as_number_new (SwfdecAsContext *context, double number)
{
  SwfdecAsObject *ret;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (context->Number != NULL, NULL);
  
  SWFDEC_AS_VALUE_SET_NUMBER (&val, number);
  ret = swfdec_as_object_create (SWFDEC_AS_FUNCTION (context->Number), 1, &val, FALSE);
  swfdec_as_context_run (context);
  return ret;
}

/*** AS CODE ***/

static void
swfdec_as_number_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double d;

  if (argc > 0) {
    d = swfdec_as_value_to_number (object->context, &argv[0]);
  } else {
    d = NAN;
  }

  if (cx->frame->construct) {
    SwfdecAsNumber *num = SWFDEC_AS_NUMBER (object);
    num->number = d;
    SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
  } else {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
  }
}

static void
swfdec_as_number_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsNumber *num = SWFDEC_AS_NUMBER (object);
  SwfdecAsValue val;
  const char *s;
  
  if (argc > 0) {
    SWFDEC_FIXME ("radix is not yet implemented");
  }
  SWFDEC_AS_VALUE_SET_NUMBER (&val, num->number);
  s = swfdec_as_value_to_string (object->context, &val);
  SWFDEC_AS_VALUE_SET_STRING (ret, s);
}

static void
swfdec_as_number_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsNumber *num = SWFDEC_AS_NUMBER (object);

  SWFDEC_AS_VALUE_SET_NUMBER (ret, num->number);
}

void
swfdec_as_number_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *number, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  number = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_Number, SWFDEC_TYPE_AS_NUMBER, swfdec_as_number_construct, 0));
  swfdec_as_native_function_set_construct_type (SWFDEC_AS_NATIVE_FUNCTION (number), SWFDEC_TYPE_AS_NUMBER);
  if (!number)
    return;
  context->Number = number;
  swfdec_as_native_function_set_object_type (SWFDEC_AS_NATIVE_FUNCTION (number), 
      SWFDEC_TYPE_AS_NUMBER);
  proto = swfdec_as_object_new (context);
  /* set the right properties on the Number object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_prototype, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_constructor, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, NAN);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_NaN, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_MAXDOUBLE);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_MAX_VALUE, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_MINDOUBLE);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_MIN_VALUE, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, -HUGE_VAL);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_NEGATIVE_INFINITY, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, HUGE_VAL);
  swfdec_as_object_set_variable (number, SWFDEC_AS_STR_POSITIVE_INFINITY, &val);
  /* set the right properties on the Number.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, number);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, SWFDEC_TYPE_AS_NUMBER, swfdec_as_number_toString, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_valueOf, SWFDEC_TYPE_AS_NUMBER, swfdec_as_number_valueOf, 0);
}

