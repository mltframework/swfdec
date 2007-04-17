/* SwfdecAs
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

#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_stack.h"
#include "swfdec_debug.h"

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
    swfdec_as_value_mark (&g_array_index (array->values, SwfdecAsValue, i));
  }

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_array_parent_class)->mark (object);
}

static void
swfdec_as_array_class_init (SwfdecAsArrayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_array_dispose;

  asobject_class->mark = swfdec_as_array_mark;
}

static void
swfdec_as_array_init (SwfdecAsArray *array)
{
  array->values = g_array_new (FALSE, TRUE, sizeof (SwfdecAsValue));
}

/**
 * swfdec_as_array_new:
 * @context: a #SwfdecAsContext
 * @preallocated_items: number of items to preallocate
 *
 * Creates a new #SwfdecAsArray. @preallocated_items is a hint on how many
 * space to reserve to avoid frequent reallocations later on.
 *
 * Returns: the new array or %NULL on OOM.
 **/
SwfdecAsObject *
swfdec_as_array_new (SwfdecAsContext *context, guint preallocated_items)
{
  guint size;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  if (preallocated_items > 1024) {
    SWFDEC_INFO ("%u items is a lot, better only preallocate 1024", preallocated_items);
    preallocated_items = 1024;
  }
  
  size = sizeof (SwfdecAsArray);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  object = g_object_new (SWFDEC_TYPE_AS_ARRAY, NULL);
  swfdec_as_object_add (object, context, size);
  if (preallocated_items) {
    g_array_set_size (SWFDEC_AS_ARRAY (object)->values, preallocated_items);
    g_array_set_size (SWFDEC_AS_ARRAY (object)->values, 0);
  }
  return object;
}

