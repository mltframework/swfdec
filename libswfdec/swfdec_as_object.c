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

#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"


typedef struct {
  const char *		variable;	/* string naming the variable */
  guint			flags;		/* SwfdecAsVariableFlag values */
  SwfdecAsValue		value;		/* value of property */
} SwfdecAsObjectVariable;

G_DEFINE_TYPE (SwfdecAsObject, swfdec_as_object, G_TYPE_OBJECT)

static void
swfdec_as_object_dispose (GObject *gobject)
{
  SwfdecAsObject *object = SWFDEC_AS_OBJECT (gobject);

  g_assert (object->properties == NULL);

  G_OBJECT_CLASS (swfdec_as_object_parent_class)->dispose (gobject);
}

static void
swfdec_as_object_mark_property (gpointer key, gpointer value, gpointer unused)
{
  SwfdecAsObjectVariable *var = value;

  swfdec_as_string_mark (key);
  swfdec_as_value_mark (&var->value);
}

static void
swfdec_as_object_do_mark (SwfdecAsObject *object)
{
  g_hash_table_foreach (object->properties, swfdec_as_object_mark_property, NULL);
}

static void
swfdec_as_object_class_init (SwfdecAsObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_object_dispose;

  klass->mark = swfdec_as_object_do_mark;
}

static void
swfdec_as_object_init (SwfdecAsObject *object)
{
}

SwfdecAsObject *
swfdec_as_object_new (SwfdecAsContext *context)
{
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsObject)))
    return NULL;
  object = g_object_new (SWFDEC_TYPE_AS_OBJECT, NULL);
  swfdec_as_object_add (object, context, sizeof (SwfdecAsObject));
  g_object_unref (object);
  return object;
}

void
swfdec_as_object_add (SwfdecAsObject *object, SwfdecAsContext *context, gsize size)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (object->properties == NULL);

  object->context = context;
  object->size = size;
  g_hash_table_insert (context->objects, object, object);
  object->properties = g_hash_table_new (g_direct_hash, g_direct_equal);
  g_object_ref (object);
}

static void
swfdec_as_object_free_property (gpointer key, gpointer value, gpointer data)
{
  SwfdecAsObject *object = data;

  swfdec_as_context_unuse_mem (object->context, sizeof (SwfdecAsObjectVariable));
  g_slice_free (SwfdecAsObjectVariable, value);
}

void
swfdec_as_object_collect (SwfdecAsObject *object)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (object->properties != NULL);

  g_hash_table_foreach (object->properties, swfdec_as_object_free_property, object);
  g_hash_table_destroy (object->properties);
  object->properties = NULL;
  swfdec_as_context_unuse_mem (object->context, object->size);
  g_object_unref (object);
}

void
swfdec_as_object_root (SwfdecAsObject *object)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail ((object->flags & SWFDEC_AS_GC_ROOT) == 0);

  object->flags |= SWFDEC_AS_GC_ROOT;
}

void
swfdec_as_object_unroot (SwfdecAsObject *object)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail ((object->flags & SWFDEC_AS_GC_ROOT) != 0);

  object->flags &= ~SWFDEC_AS_GC_ROOT;
}

void
swfdec_as_object_set_variable (SwfdecAsObject *object,
    const SwfdecAsValue *variable, const SwfdecAsValue *value)
{
  const char *s;
  SwfdecAsObjectVariable *var;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_VALUE (variable));
  g_return_if_fail (SWFDEC_IS_AS_VALUE (value));

  s = swfdec_as_value_to_string (object->context, variable);
  var = g_hash_table_lookup (object->properties, s);
  if (var == NULL) {
    if (!swfdec_as_context_use_mem (object->context, sizeof (SwfdecAsObjectVariable)))
      return;
    var = g_slice_new0 (SwfdecAsObjectVariable);
    var->variable = s;
    g_hash_table_insert (object->properties, (gpointer) s, var);
  } else {
    if (var->flags & SWFDEC_AS_VARIABLE_READONLY)
      return;
  }
  var->value = *value;
}

void
swfdec_as_object_get_variable (SwfdecAsObject *object, 
    const SwfdecAsValue *variable, SwfdecAsValue *value)
{
  const char *s;
  SwfdecAsObjectVariable *var;
  guint i;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_VALUE (variable));
  g_return_if_fail (value != NULL);

  s = swfdec_as_value_to_string (object->context, variable);
  for (i = 0; i < 256 && object != NULL; i++) {
    var = g_hash_table_lookup (object->properties, s);
    if (var == NULL) {
      object = object->prototype;
      continue;
    }
    *value = var->value;
    return;
  }
  if (i == 256) {
    swfdec_as_context_abort (object->context, "Prototype recursion limit exceeded");
    return;
  }
  SWFDEC_AS_VALUE_SET_UNDEFINED (value);
}

void
swfdec_as_object_delete_variable (SwfdecAsObject *object, 
    const SwfdecAsValue *variable)
{
  const char *s;
  SwfdecAsObjectVariable *var;
  guint i;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_VALUE (variable));

  s = swfdec_as_value_to_string (object->context, variable);
  for (i = 0; i < 256 && object != NULL; i++) {
    var = g_hash_table_lookup (object->properties, s);
    if (var == NULL) {
      object = object->prototype;
      continue;
    }
    if (var->flags & SWFDEC_AS_VARIABLE_PERMANENT)
      return;
    g_hash_table_remove (object->properties, s);
    swfdec_as_object_free_property ((gpointer) s, var, object);
    return;
  }
  if (i == 256) {
    swfdec_as_context_abort (object->context, "Prototype recursion limit exceeded");
    return;
  }
}

