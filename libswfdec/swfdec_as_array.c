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

#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_function.h"
#include "swfdec_debug.h"

typedef struct {
  guint			id;
  SwfdecAsVariable	variable;
} SwfdecAsArrayEntry;

G_DEFINE_TYPE (SwfdecAsArray, swfdec_as_array, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_array_dispose (GObject *object)
{
  SwfdecAsArray *array = SWFDEC_AS_ARRAY (object);

  g_array_free (array->values, TRUE);

  G_OBJECT_CLASS (swfdec_as_array_parent_class)->dispose (object);
}

static void
swfdec_as_array_mark (SwfdecAsObject *object)
{
  SwfdecAsArray *array = SWFDEC_AS_ARRAY (object);
  guint i;

  for (i = 0; i < array->values->len; i++) {
    swfdec_as_variable_mark (&g_array_index (array->values, SwfdecAsArrayEntry, i).variable);
  }

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_array_parent_class)->mark (object);
}

/* finds the biggest element < the desired element */
static int
compare_array_entry (gconstpointer keyp, gconstpointer arrp)
{
  const SwfdecAsArrayEntry *key = keyp;
  const SwfdecAsArrayEntry *arr = arrp;

  if (arr->id > key->id)
    return -1;
  arr++;
  if (arr->id == 0 || arr->id > key->id)
    return 0;
  return 1;
}

/* returns the smallest element >= the desired element
 * The returned element may be the zeroed element after the array
 */
static SwfdecAsArrayEntry *
swfdec_as_array_find (GArray *array, guint id)
{
  SwfdecAsArrayEntry entry = { id, };
  SwfdecAsArrayEntry *ret;

  g_assert (array->len > 0);
  ret = bsearch (&entry, array->data, array->len, sizeof (SwfdecAsArrayEntry), compare_array_entry);
  ret++;
  return ret;
}

static SwfdecAsVariable *
swfdec_as_array_get_variable (SwfdecAsArray *array, guint index, gboolean create)
{
  if (array->values->len == 0 ||
      index >= array->length) {
    if (create) {
      SwfdecAsArrayEntry append = { index, };
      g_array_append_val (array->values, append);
      /* FIXME: handle pushing in length == G_MAXINT case */
      array->length = index + 1;
      return &g_array_index (array->values, SwfdecAsArrayEntry, array->values->len - 1).variable;
    } else {
      return NULL;
    }
  } else {
    SwfdecAsArrayEntry *entry = swfdec_as_array_find (array->values, index);
    if (entry->id == index) {
      return &entry->variable;
    } else if (create) {
      guint pos = (guint) (entry - (SwfdecAsArrayEntry *) array->values->data);
      SwfdecAsArrayEntry append = { index, };
      g_array_insert_val (array->values, pos, append);
      return &g_array_index (array->values, SwfdecAsArrayEntry, array->values->len - 1).variable;
    } else {
      return NULL;
    }
  }
}

static SwfdecAsVariable *
swfdec_as_array_get (SwfdecAsObject *object, const char *variable, gboolean create)
{
  return SWFDEC_AS_OBJECT_CLASS (swfdec_as_array_parent_class)->get (object, variable, create);
}

#if 0
  /* delete the variable - it does exists */
  void			(* delete)		(SwfdecAsObject *       object,
						 const char *		variable);
  /* call with every variable until func returns FALSE */
  gboolean		(* foreach)		(SwfdecAsObject *	object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
#endif
static void
swfdec_as_array_class_init (SwfdecAsArrayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_array_dispose;

  asobject_class->mark = swfdec_as_array_mark;
  asobject_class->get = swfdec_as_array_get;
}

static void
swfdec_as_array_init (SwfdecAsArray *array)
{
  array->values = g_array_new (TRUE, FALSE, sizeof (SwfdecAsArrayEntry));
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
swfdec_as_array_set_value (SwfdecAsArray *array, guint index, const SwfdecAsValue *value)
{
  SwfdecAsVariable *var;

  g_return_if_fail (SWFDEC_IS_AS_ARRAY (array));
  g_return_if_fail (index < G_MAXINT32);
  g_return_if_fail (value != NULL);

  var = swfdec_as_array_get_variable (array, index, TRUE);
  if (var == NULL)
    return;
  swfdec_as_variable_set (SWFDEC_AS_OBJECT (array), var, value);
}

void
swfdec_as_array_get_value (SwfdecAsArray *array, guint index, SwfdecAsValue *value)
{
  SwfdecAsVariable *var;

  g_return_if_fail (SWFDEC_IS_AS_ARRAY (array));
  g_return_if_fail (index < G_MAXINT32); 
  g_return_if_fail (value != NULL);

  var = swfdec_as_array_get_variable (array, index, FALSE);
  if (var == NULL) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (value);
  } else {
    swfdec_as_variable_get (SWFDEC_AS_OBJECT (array), var, value);
  }
}

/*** AS CODE ***/

static void
swfdec_as_array_construct (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{

}


void
swfdec_as_array_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *array, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  array = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global, 
      SWFDEC_AS_STR_Array, 0, swfdec_as_array_construct, 0));
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
}

