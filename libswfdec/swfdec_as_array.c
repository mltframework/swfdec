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

#include <stdlib.h>
#include <string.h>

#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_function.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsArray, swfdec_as_array, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_array_dispose (GObject *object)
{
  //SwfdecAsArray *array = SWFDEC_AS_ARRAY (object);

  G_OBJECT_CLASS (swfdec_as_array_parent_class)->dispose (object);
}

/* NB: type is important for overflow */
static inline gint32
swfdec_as_array_to_index (const char *str)
{
  char *end;
  gulong l;
  
  l = strtoul (str, &end, 10);
  if (*end != 0 || l > G_MAXINT32)
    return -1;
  return l;
}

static gint32
swfdec_as_array_get_length (SwfdecAsObject *array)
{
  SwfdecAsValue val;
  gint32 length;

  swfdec_as_object_get_variable (array, SWFDEC_AS_STR_length, &val);
  length = swfdec_as_value_to_integer (array->context, &val);
  if (length < 0)
    return 0;
  else
    return length;
}

static void
swfdec_as_array_set (SwfdecAsObject *object, const char *variable, const SwfdecAsValue *val)
{
  gint32 l = swfdec_as_array_to_index (variable);

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_array_parent_class)->set (object, variable, val);
  if (l > -1) {
    int cur;
    l++;
    cur = swfdec_as_array_get_length (object);
    if (l > cur) {
      SwfdecAsValue val;
      SWFDEC_AS_VALUE_SET_INT (&val, l);
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_length, &val);
    }
  }
}

static void
swfdec_as_array_class_init (SwfdecAsArrayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_array_dispose;

  asobject_class->set = swfdec_as_array_set;
}

static void
swfdec_as_array_init (SwfdecAsArray *array)
{
}

/**
 * swfdec_as_array_new:
 * @context: a #SwfdecAsContext
 *
 * Creates a new #SwfdecAsArray. This is the same as executing the Actionscript
 * code "new Array ()"
 *
 * Returns: the new array or %NULL on OOM.
 **/
SwfdecAsObject *
swfdec_as_array_new (SwfdecAsContext *context)
{
  SwfdecAsObject *ret;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (context->Array != NULL, NULL);
  
  ret = swfdec_as_object_create (SWFDEC_AS_FUNCTION (context->Array), 0, NULL);
  swfdec_as_object_root (ret);
  swfdec_as_context_run (context);
  swfdec_as_object_unroot (ret);
  return ret;
}

void
swfdec_as_array_append (SwfdecAsArray *array, guint n, const SwfdecAsValue *value)
{
  SwfdecAsObject *object;
  guint i;
  gint32 length;

  g_return_if_fail (SWFDEC_IS_AS_ARRAY (array));
  g_return_if_fail (n > 0);
  g_return_if_fail (value != NULL);

  object = SWFDEC_AS_OBJECT (array);
  length = swfdec_as_array_get_length (object);
  for (i = 0; i < n; i++) {
    const char *var = swfdec_as_double_to_string (object->context, length);
    swfdec_as_object_set_variable (object, var, &value[i]);
    length++;
  }
}

/*** AS CODE ***/

static void
swfdec_as_array_toString (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsContext *cx = object->context;
  int i, length;
  const char *s, *str;
  SwfdecAsValue val;

  length = swfdec_as_array_get_length (object);
  if (length > 0) {
    s = swfdec_as_double_to_string (cx, 0);
    swfdec_as_object_get_variable (object, s, &val);
    str = swfdec_as_value_to_string (cx, &val);
    for (i = 1; i < length; i++) {
      s = swfdec_as_double_to_string (cx, i);
      swfdec_as_object_get_variable (object, s, &val);
      s = swfdec_as_value_to_string (cx, &val);
      str = swfdec_as_str_concat (cx, str, SWFDEC_AS_STR_COMMA);
      str = swfdec_as_str_concat (cx, str, s);
    }
  } else {
    str = SWFDEC_AS_STR_EMPTY;
  }
  SWFDEC_AS_VALUE_SET_STRING (ret, str);
}

void
swfdec_as_array_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *array, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  array = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global, 
      SWFDEC_AS_STR_Array, 0, NULL, 0));
  if (!array)
    return;
  context->Array = array;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsArray)))
    return;
  proto = g_object_new (SWFDEC_TYPE_AS_ARRAY, NULL);
  swfdec_as_object_add (proto, context, sizeof (SwfdecAsArray));
  /* set the right properties on the Array object */
  swfdec_as_object_root (proto);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_prototype, &val);
  swfdec_as_object_unroot (proto);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_constructor, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, 1);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_CASEINSENSITIVE, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, 2);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_DESCENDING, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, 4);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_UNIQUESORT, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, 8);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_RETURNINDEXEDARRAY, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, 16);
  swfdec_as_object_set_variable (array, SWFDEC_AS_STR_NUMERIC, &val);
  /* set the right properties on the Array.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, array);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, 0, swfdec_as_array_toString, 0);
}

