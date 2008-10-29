/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_as_object.h"

#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_relay.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_string.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"
#include "swfdec_utils.h"

/**
 * SECTION:SwfdecAsObject
 * @title: SwfdecAsObject
 * @short_description: the base object type for scriptable objects
 *
 * This is the basic object type in Swfdec. Every object used by the script 
 * engine must be a #SwfdecAsObject. It handles memory management and assigning
 * variables to it. Almost all functions that are called on objects require that
 * the objects have been added to the garbage collector previously. For 
 * custom-created objects, you need to do this using swfdec_as_object_add(), 
 * built-in functions that create objects do this manually.
 *
 * Note that you cannot know the lifetime of a #SwfdecAsObject, since scripts 
 * may assign it as a variable to other objects. So you should not assume to 
 * know when an object gets removed.
 */

/**
 * SwfdecAsObject:
 *
 * Every object value inside the Swfdec script engine must be a SwfdecAsObject.
 * If you want to add custom objects to your script engine, you need to create a
 * subclass. The class provides a number of virtual functions that you can 
 * override to achieve the desired behaviour.
 */

/**
 * SwfdecAsVariableFlag:
 * @SWFDEC_AS_VARIABLE_HIDDEN: Do not include variable in enumerations and
 *                                swfdec_as_object_foreach().
 * @SWFDEC_AS_VARIABLE_PERMANENT: Do not allow swfdec_as_object_delete_variable()
 *                                to delete this variable.
 * @SWFDEC_AS_VARIABLE_CONSTANT: Do not allow changing the value with
 *                               swfdec_as_object_set_variable().
 * @SWFDEC_AS_VARIABLE_VERSION_6_UP: This symbol is only visible in version 6 
 *                                   and above.
 * @SWFDEC_AS_VARIABLE_VERSION_NOT_6: This symbols is visible in all versions 
 *                                    but version 6.
 * @SWFDEC_AS_VARIABLE_VERSION_7_UP: This symbol is only visible in version 7 
 *                                   and above.
 * @SWFDEC_AS_VARIABLE_VERSION_8_UP: This symbol is only visible in version 8 
 *                                   and above.
 * @SWFDEC_AS_VARIABLE_VERSION_9_UP: This symbol is only visible in version 9 
 *                                   and above.
 *
 * These flags are used to describe various properties of a variable inside
 * Swfdec. You can manually set them with swfdec_as_object_set_variable_flags().
 */

/**
 * SwfdecAsDeleteReturn:
 * @SWFDEC_AS_DELETE_NOT_FOUND: The variable was not found and therefore 
 *                              couldn't be deleted.
 * @SWFDEC_AS_DELETE_DELETED: The variable was deleted.
 * @SWFDEC_AS_DELETE_NOT_DELETED: The variable was found but could not be 
 *                                deleted.
 *
 * This is the return value used by swfdec_as_object_delete_variable(). It 
 * describes the various outcomes of trying to delete a variable.
 */

/**
 * SwfdecAsVariableForeach:
 * @object: The object this function is run on
 * @variable: garbage-collected name of the current variables
 * @value: value of the current variable
 * @flags: Flags associated with the current variable
 * @data: User data passed to swfdec_as_object_foreach()
 *
 * Function prototype for the swfdec_as_object_foreach() function.
 *
 * Returns: %TRUE to continue running the foreach function, %FALSE to stop
 */

typedef struct _SwfdecAsVariable SwfdecAsVariable;
struct _SwfdecAsVariable {
  guint			flags;		/* SwfdecAsVariableFlag values */
  SwfdecAsValue     	value;		/* value of property */
  SwfdecAsFunction *	get;		/* getter set with swfdec_as_object_add_property */
  SwfdecAsFunction *	set;		/* setter or %NULL */
};

typedef struct {
  SwfdecAsFunction *	watch;		/* watcher or %NULL */
  SwfdecAsValue		watch_data;	/* user data to watcher */
  guint			refcount;	/* refcount - misused for recursion detection */
} SwfdecAsWatch;

G_DEFINE_TYPE (SwfdecAsObject, swfdec_as_object, SWFDEC_TYPE_GC_OBJECT)

static gboolean
swfdec_as_watch_can_recurse (SwfdecAsWatch *watch)
{
  guint version;

  version = swfdec_gc_object_get_context (watch->watch)->version;
  if (version <= 6) {
    return watch->refcount <= 1;
  } else {
    return watch->refcount <= 64 + 1;
  }
}

static void
swfdec_as_watch_ref (SwfdecAsWatch *watch)
{
  watch->refcount++;
}

static void
swfdec_as_watch_unref (SwfdecAsWatch *watch)
{
  watch->refcount--;
  if (watch->refcount == 0) {
    swfdec_as_context_unuse_mem (swfdec_gc_object_get_context (watch->watch), 
	sizeof (SwfdecAsWatch));
    g_slice_free (SwfdecAsWatch, watch);
  }
}

/* This is a huge hack design-wise, but we can't use watch->watch, 
 * it might be gone already */
static gboolean
swfdec_as_object_steal_watches (gpointer key, gpointer value, gpointer object)
{
  SwfdecAsWatch *watch = value;

  g_assert (watch->refcount == 1);
  watch->watch = (SwfdecAsFunction *) object;
  swfdec_as_watch_unref (watch);
  return TRUE;
}

static void
swfdec_as_object_free_property (gpointer key, gpointer value, gpointer data)
{
  SwfdecAsObject *object = data;

  swfdec_as_context_unuse_mem (swfdec_gc_object_get_context (object), sizeof (SwfdecAsVariable));
  g_slice_free (SwfdecAsVariable, value);
}

static void
swfdec_as_object_dispose (GObject *gobject)
{
  SwfdecAsContext *context = swfdec_gc_object_get_context (gobject);
  SwfdecAsObject *object = SWFDEC_AS_OBJECT (gobject);

  if (context->debugger) {
    SwfdecAsDebuggerClass *klass = SWFDEC_AS_DEBUGGER_GET_CLASS (context->debugger);
    if (klass->remove)
      klass->remove (context->debugger, context, object);
  }

  if (object->properties) {
    g_hash_table_foreach (object->properties, swfdec_as_object_free_property, object);
    g_hash_table_destroy (object->properties);
    object->properties = NULL;
  }
  if (object->watches) {
    g_hash_table_foreach_steal (object->watches, swfdec_as_object_steal_watches, object);
    g_hash_table_destroy (object->watches);
    object->watches = NULL;
  }
  g_slist_free (object->interfaces);
  object->interfaces = NULL;

  G_OBJECT_CLASS (swfdec_as_object_parent_class)->dispose (gobject);
}

static void
swfdec_gc_object_mark_property (gpointer key, gpointer value, gpointer unused)
{
  SwfdecAsVariable *var = value;

  swfdec_as_string_mark (key);
  if (var->get) {
    swfdec_gc_object_mark (var->get);
    if (var->set)
      swfdec_gc_object_mark (var->set);
  } else {
    swfdec_as_value_mark (&var->value);
  }
}

static void
swfdec_gc_object_mark_watch (gpointer key, gpointer value, gpointer unused)
{
  SwfdecAsWatch *watch = value;

  swfdec_as_string_mark (key);
  swfdec_gc_object_mark (watch->watch);
  swfdec_as_value_mark (&watch->watch_data);
}

static void
swfdec_as_object_mark (SwfdecGcObject *gc)
{
  SwfdecAsObject *object = SWFDEC_AS_OBJECT (gc);

  if (object->prototype)
    swfdec_gc_object_mark (object->prototype);
  g_hash_table_foreach (object->properties, swfdec_gc_object_mark_property, NULL);
  if (object->watches)
    g_hash_table_foreach (object->watches, swfdec_gc_object_mark_watch, NULL);
  if (object->relay)
    swfdec_gc_object_mark (object->relay);
  g_slist_foreach (object->interfaces, (GFunc) swfdec_gc_object_mark, NULL); 
}

static gboolean
swfdec_as_object_lookup_case_insensitive (gpointer key, gpointer value, gpointer user_data)
{
  return g_ascii_strcasecmp (key, user_data) == 0;
}

static gboolean
swfdec_as_variable_name_is_valid (const char *name)
{
  return name != SWFDEC_AS_STR_EMPTY;
}

static SwfdecAsVariable *
swfdec_as_object_hash_lookup (SwfdecAsObject *object, const char *variable)
{
  SwfdecAsVariable *var = g_hash_table_lookup (object->properties, variable);

  if (var || swfdec_gc_object_get_context (object)->version >= 7)
    return var;
  var = g_hash_table_find (object->properties, swfdec_as_object_lookup_case_insensitive, (gpointer) variable);
  return var;
}

static SwfdecAsVariable *
swfdec_as_object_hash_create (SwfdecAsObject *object, const char *variable, guint flags)
{
  SwfdecAsVariable *var;

  if (!swfdec_as_variable_name_is_valid (variable))
    return NULL;
  swfdec_as_context_use_mem (swfdec_gc_object_get_context (object), sizeof (SwfdecAsVariable));
  var = g_slice_new0 (SwfdecAsVariable);
  var->flags = flags;
  g_hash_table_insert (object->properties, (gpointer) variable, var);

  return var;
}

static gboolean
swfdec_as_object_variable_enabled_in_version (SwfdecAsVariable *var,
    guint version)
{
  if (var->flags & SWFDEC_AS_VARIABLE_VERSION_6_UP && version < 6)
    return FALSE;
  if (var->flags & SWFDEC_AS_VARIABLE_VERSION_NOT_6 && version == 6)
    return FALSE;
  if (var->flags & SWFDEC_AS_VARIABLE_VERSION_7_UP && version < 7)
    return FALSE;
  if (var->flags & SWFDEC_AS_VARIABLE_VERSION_8_UP && version < 8)
    return FALSE;
  if (var->flags & SWFDEC_AS_VARIABLE_VERSION_9_UP && version < 9)
    return FALSE;

  return TRUE;
}

static gboolean
swfdec_as_object_do_get (SwfdecAsObject *object, SwfdecAsObject *orig,
    const char *variable, SwfdecAsValue *val, guint *flags)
{
  SwfdecAsVariable *var;
  
  var = swfdec_as_object_hash_lookup (object, variable);
  if (var == NULL)
    return FALSE;

  /* variable flag checks */
  if (!swfdec_as_object_variable_enabled_in_version (var,
	swfdec_gc_object_get_context (object)->version))
    return FALSE;

  if (var->get) {
    swfdec_as_function_call (var->get, orig, 0, NULL, val);
    *flags = var->flags;
  } else {
    *val = var->value;
    *flags = var->flags;
  }
  return TRUE;
}

static SwfdecAsWatch *
swfdec_as_watch_new (SwfdecAsFunction *function)
{
  SwfdecAsWatch *watch;

  swfdec_as_context_use_mem (swfdec_gc_object_get_context (function), 
      sizeof (SwfdecAsWatch));

  watch = g_slice_new (SwfdecAsWatch);
  watch->refcount = 1;
  watch->watch = function;
  SWFDEC_AS_VALUE_SET_UNDEFINED (&watch->watch_data);
  return watch;
}

/*
 * Like swfdec_as_object_get_prototype, but doesn't check 8_UP flag when
 * version is 7 and doesn't check if the property has been deleted if version
 * is 6 or earlier
 */
static SwfdecAsObject *
swfdec_as_object_get_prototype_internal (SwfdecAsObject *object)
{
  int version;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);

  version = swfdec_gc_object_get_context (object)->version;

  if (object->prototype == NULL)
    return NULL;

  if (object->prototype_flags & SWFDEC_AS_VARIABLE_VERSION_6_UP && version < 6)
    return NULL;
  // don't check for NOT_6 flag
  if (object->prototype_flags & SWFDEC_AS_VARIABLE_VERSION_7_UP && version < 7)
    return NULL;
  // don't check 8_UP or 9_UP for version 6, 7 or 8
  if (object->prototype_flags & (SWFDEC_AS_VARIABLE_VERSION_8_UP | SWFDEC_AS_VARIABLE_VERSION_9_UP) && version < 6)
    return NULL;
  // check that it exists, if version < 7
  if (version < 7 &&
      !swfdec_as_object_hash_lookup (object, SWFDEC_AS_STR___proto__))
    return NULL;

  return object->prototype;
}

/*
 * Get's the object->prototype, if propflags allow it for current version and
 * if it hasn't been deleted from the object already
 */
SwfdecAsObject *
swfdec_as_object_get_prototype (SwfdecAsObject *object)
{
  int version;
  SwfdecAsObject *prototype;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);

  version = swfdec_gc_object_get_context (object)->version;

  prototype = swfdec_as_object_get_prototype_internal (object);

  if (prototype == NULL)
    return NULL;
  // check 8_UP for version 7, still not for version 6
  if (object->prototype_flags & SWFDEC_AS_VARIABLE_VERSION_8_UP &&
      version == 7)
    return NULL;
  // check 9_UP flag for version 8, still not for version 7 or 6
  if (object->prototype_flags & SWFDEC_AS_VARIABLE_VERSION_9_UP &&
      version == 8)
    return NULL;
  // require it to exist even on version >= 7
  if (version >= 7 &&
      !swfdec_as_object_hash_lookup (object, SWFDEC_AS_STR___proto__))
    return NULL;

  return object->prototype;
}

static SwfdecAsVariable *
swfdec_as_object_hash_lookup_with_prototype (SwfdecAsObject *object,
    const char *variable, SwfdecAsObject **proto)
{
  SwfdecAsVariable *var;
  SwfdecAsObject *proto_;

  g_return_val_if_fail (swfdec_as_variable_name_is_valid (variable), NULL);

  proto_ = NULL;

  // match first level variable even if it has version flags that hide it in
  // this version
  var = swfdec_as_object_hash_lookup (object, variable);
  if (var == NULL && variable != SWFDEC_AS_STR___proto__) {
    guint i;

    proto_ = swfdec_as_object_get_prototype (object);

    for (i = 0; i < SWFDEC_AS_OBJECT_PROTOTYPE_RECURSION_LIMIT && proto_; i++) {
      var = swfdec_as_object_hash_lookup (proto_, variable);
      if (var && var->get)
	break;
      proto_ = swfdec_as_object_get_prototype (proto_);
      var = NULL;
    }

    if (i == SWFDEC_AS_OBJECT_PROTOTYPE_RECURSION_LIMIT) {
      swfdec_as_context_abort (swfdec_gc_object_get_context (object), "Prototype recursion limit exceeded");
      return NULL;
    }
  }

  if (proto != NULL)
    *proto = proto_;

  return var;
}

#if 0
static void
swfdec_as_array_set (SwfdecAsObject *object, const char *variable,
    const SwfdecAsValue *val, guint flags)
{
}
#endif

static void
swfdec_as_object_do_set (SwfdecAsObject *object, const char *variable, 
    const SwfdecAsValue *val, guint flags)
{
  SwfdecAsVariable *var;
  SwfdecAsWatch *watch;
  SwfdecAsObject *proto;
  SwfdecAsContext *context;

  if (!swfdec_as_variable_name_is_valid (variable) ||
      object->super)
    return;

  context = swfdec_gc_object_get_context (object);
  var = swfdec_as_object_hash_lookup_with_prototype (object, variable, &proto);
  if (swfdec_as_context_is_aborted (context))
    return;

  // if variable is disabled in this version
  if (var != NULL && !swfdec_as_object_variable_enabled_in_version (var, context->version)) {
    if (proto == NULL) {
      // it's at the top level, remove getter and setter plus overwrite
      var->get = NULL;
      var->set = NULL;
    } else {
      // it's in proto, we create a new one at the top level
      var = NULL;
    }
  }

  if (var == NULL) {
    var = swfdec_as_object_hash_create (object, variable, flags);
    if (var == NULL)
      return;
  } else {
    if (var->flags & SWFDEC_AS_VARIABLE_CONSTANT)
      return;
    // remove the flags that could make this variable hidden
    if (context->version == 6) {
      // version 6, so let's forget SWFDEC_AS_VARIABLE_VERSION_7_UP flag, oops!
      // we will still set the value though, even if that flag is set
      var->flags &= ~(SWFDEC_AS_VARIABLE_VERSION_6_UP |
	  SWFDEC_AS_VARIABLE_VERSION_NOT_6 | SWFDEC_AS_VARIABLE_VERSION_8_UP |
	  SWFDEC_AS_VARIABLE_VERSION_9_UP);
    } else {
      var->flags &= ~(SWFDEC_AS_VARIABLE_VERSION_6_UP |
	  SWFDEC_AS_VARIABLE_VERSION_NOT_6 | SWFDEC_AS_VARIABLE_VERSION_7_UP |
	  SWFDEC_AS_VARIABLE_VERSION_8_UP | SWFDEC_AS_VARIABLE_VERSION_9_UP);
    }
  }
  if (object->watches) {
    SwfdecAsValue ret = *val;
    watch = g_hash_table_lookup (object->watches, variable);
    /* FIXME: figure out if this limit here is correct. Add a watch in Flash 7 
     * and set a variable using Flash 6 */
    if (watch && swfdec_as_watch_can_recurse (watch)) {
      SwfdecAsValue args[4];
      SWFDEC_AS_VALUE_SET_STRING (&args[0], variable);
      args[1] = var->value;
      args[2] = *val;
      args[3] = watch->watch_data;
      swfdec_as_watch_ref (watch);
      swfdec_as_function_call (watch->watch, object, 4, args, &ret);
      swfdec_as_watch_unref (watch);
      var = swfdec_as_object_hash_lookup_with_prototype (object, variable,
	  NULL);
      if (swfdec_as_context_is_aborted (context))
	return;
      if (var == NULL) {
	SWFDEC_INFO ("watch removed variable %s", variable);
	return;
      }
    }

    var->value = ret;
  } else {
    watch = NULL;
  }

  if (object->array) {
    /* if we changed to smaller length, destroy all values that are outside it */
    if (!swfdec_strcmp (context->version, variable,
	  SWFDEC_AS_STR_length))
    {
      gint32 length_old = swfdec_as_array_get_length (object);
      gint32 length_new = swfdec_as_value_to_integer (context, val);
      length_new = MAX (0, length_new);
      if (length_old > length_new) {
	swfdec_as_array_remove_range (object, length_new,
	    length_old - length_new);
      }
    }
  }

  if (var->get) {
    if (var->set) {
      SwfdecAsValue tmp;
      swfdec_as_function_call (var->set, object, 1, val, &tmp);
    }
  } else if (watch == NULL) {
    var->value = *val;
  }

  if (variable == SWFDEC_AS_STR___proto__) {
    if (SWFDEC_AS_VALUE_IS_COMPOSITE (val) &&
	!SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_COMPOSITE (val))) {
      object->prototype = SWFDEC_AS_VALUE_GET_COMPOSITE (val);
      object->prototype_flags = var->flags;
    } else {
      object->prototype = NULL;
      object->prototype_flags = 0;
    }
  }

  /* If we are an array, do the special magic now */
  if (object->array) {
    char *end;
    gint32 l;

    /* if we added new value outside the current length, set a bigger length */
    l = strtoul (variable, &end, 10);
    if (*end == '\0') {
      SwfdecAsValue tmp;
      gint32 length;
      swfdec_as_object_get_variable (object, SWFDEC_AS_STR_length, &tmp);
      length = swfdec_as_value_to_integer (context, &tmp);
      if (l >= length) {
	object->array = FALSE;
	swfdec_as_value_set_integer (swfdec_gc_object_get_context (object), &tmp, l + 1);
	swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_length, &tmp,
	    SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
	object->array = TRUE;
      }
    }
  }
}

typedef struct {
  SwfdecAsObject *		object;
  SwfdecAsVariableForeach	func;
  gpointer			data;
  gboolean			retval;
} ForeachData;

static void
swfdec_as_object_hash_foreach (gpointer key, gpointer value, gpointer data)
{
  ForeachData *fdata = data;
  SwfdecAsVariable *var = value;

  if (!fdata->retval)
    return;

  fdata->retval = fdata->func (fdata->object, key, &var->value, var->flags, fdata->data);
}

/* FIXME: does not do Adobe Flash's order for Enumerate actions */
static gboolean
swfdec_as_object_do_foreach (SwfdecAsObject *object, SwfdecAsVariableForeach func, gpointer data)
{
  ForeachData fdata = { object, func, data, TRUE };

  g_hash_table_foreach (object->properties, swfdec_as_object_hash_foreach, &fdata);
  return fdata.retval;
}

typedef struct {
  SwfdecAsObject *		object;
  SwfdecAsVariableForeachRemove	func;
  gpointer			data;
} ForeachRemoveData;

static gboolean
swfdec_as_object_hash_foreach_remove (gpointer key, gpointer value, gpointer data)
{
  ForeachRemoveData *fdata = data;
  SwfdecAsVariable *var = value;

  if (!fdata->func (fdata->object, key, &var->value, var->flags, fdata->data))
    return FALSE;

  swfdec_as_object_free_property (NULL, var, fdata->object);
  return TRUE;
}

/**
 * swfdec_as_object_foreach_remove:
 * @object: a #SwfdecAsObject
 * @func: function that determines which object to remove
 * @data: data to pass to @func
 *
 * Removes all variables form @object where @func returns %TRUE. This is an 
 * internal function for array operations.
 *
 * Returns: he number of variables removed
 **/
guint
swfdec_as_object_foreach_remove (SwfdecAsObject *object, SwfdecAsVariableForeach func,
    gpointer data)
{
  ForeachRemoveData fdata = { object, func, data };

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), 0);
  g_return_val_if_fail (func != NULL, 0);

  return g_hash_table_foreach_remove (object->properties,
      swfdec_as_object_hash_foreach_remove, &fdata);
}

typedef struct {
  SwfdecAsObject *		object;
  GHashTable *			properties_new;
  SwfdecAsVariableForeachRename	func;
  gpointer			data;
} ForeachRenameData;

static gboolean
swfdec_as_object_hash_foreach_rename (gpointer key, gpointer value, gpointer data)
{
  ForeachRenameData *fdata = data;
  SwfdecAsVariable *var = value;
  const char *key_new;

  key_new = fdata->func (fdata->object, key, &var->value, var->flags, fdata->data);
  if (key_new) {
    g_hash_table_insert (fdata->properties_new, (gpointer) key_new, var);
  } else {
    swfdec_as_object_free_property (NULL, var, fdata->object);
  }

  return TRUE;
}

/**
 * swfdec_as_object_foreach_rename:
 * @object: a #SwfdecAsObject
 * @func: function determining the new name
 * @data: data to pass to @func
 *
 * Calls @func for each variable of @object. The function is then supposed 
 * to return the new name of the variable or %NULL if the variable should be 
 * removed. This is an internal function for array operations.
 **/
void
swfdec_as_object_foreach_rename (SwfdecAsObject *object, SwfdecAsVariableForeachRename func,
    gpointer data)
{
  ForeachRenameData fdata = { object, NULL, func, data };

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (func != NULL);

  fdata.properties_new = g_hash_table_new (g_direct_hash, g_direct_equal);
  g_hash_table_foreach_remove (object->properties, swfdec_as_object_hash_foreach_rename, &fdata);
  g_hash_table_destroy (object->properties);
  object->properties = fdata.properties_new;
}

static GObject *
swfdec_as_object_constructor (GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties)
{
  GObject *gobject;
  SwfdecAsContext *context;

  gobject = G_OBJECT_CLASS (swfdec_as_object_parent_class)->constructor (type, 
      n_construct_properties, construct_properties);

  context = swfdec_gc_object_get_context (gobject);
  if (context->debugger) {
    SwfdecAsDebuggerClass *dklass = SWFDEC_AS_DEBUGGER_GET_CLASS (context->debugger);
    if (dklass->add)
      dklass->add (context->debugger, context, SWFDEC_AS_OBJECT (gobject));
  }

  return gobject;
}

static void
swfdec_as_object_class_init (SwfdecAsObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  object_class->constructor = swfdec_as_object_constructor;
  object_class->dispose = swfdec_as_object_dispose;

  gc_class->mark = swfdec_as_object_mark;

  klass->get = swfdec_as_object_do_get;
  klass->set = swfdec_as_object_do_set;
  klass->foreach = swfdec_as_object_do_foreach;
}

static void
swfdec_as_object_init (SwfdecAsObject *object)
{
  object->properties = g_hash_table_new (g_direct_hash, g_direct_equal);
}

/**
 * swfdec_as_object_new_empty:
 * @context: a #SwfdecAsContext
 *
 * Creates an empty object. The prototype and constructor properties of the
 * returned object will not be set. You probably want to call 
 * swfdec_as_object_set_constructor() on the returned object yourself.
 * You may want to use swfdec_as_object_new() instead.
 *
 * Returns: A new #SwfdecAsObject added to @context
 **/
SwfdecAsObject *
swfdec_as_object_new_empty (SwfdecAsContext *context)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  
  return swfdec_as_object_new (context, NULL);
}

/**
 * swfdec_as_object_new:
 * @context: a #SwfdecAsContext
 * @...: %NULL-terminated list of names of the constructor or %NULL for an 
 *       empty object.
 *
 * Allocates a new object and if @name is not %NULL, runs the constructor.
 * Name is a list of variables to get from the global context as the 
 * constructor.
 *
 * Returns: the new object
 **/
SwfdecAsObject *
swfdec_as_object_new (SwfdecAsContext *context, ...)
{
  SwfdecAsObject *object, *fun;
  SwfdecAsValue rval;
  const char *name;
  va_list args;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  
  object = g_object_new (SWFDEC_TYPE_AS_OBJECT, "context", context, NULL);
  va_start (args, context);
  name = va_arg (args, const char *);
  if (name == NULL)
    return object;

  g_return_val_if_fail (context->global, NULL);

  fun = swfdec_as_object_set_constructor_by_namev (object, name, args);
  va_end (args);
  if (SWFDEC_IS_AS_FUNCTION (fun->relay)) {
    swfdec_as_function_call_full (SWFDEC_AS_FUNCTION (fun->relay), object,
	TRUE, object->prototype, 0, NULL, &rval);
  }
  return object;
}

/**
 * swfdec_as_object_set_variable:
 * @object: a #SwfdecAsObject
 * @variable: garbage-collected name of the variable to set
 * @value: value to set the variable to
 *
 * Sets a variable on @object. It is not guaranteed that getting the variable
 * after setting it results in the same value. This is a mcaro that calls 
 * swfdec_as_object_set_variable_and_flags()
 **/
/**
 * swfdec_as_object_set_variable_and_flags:
 * @object: a #SwfdecAsObject
 * @variable: garbage-collected name of the variable to set
 * @value: value to set the variable to
 * @default_flags: flags to use if creating the variable anew - the flags will
 *                 be ignored if the property already exists.
 *
 * Sets a variable on @object. It is not guaranteed that getting the variable
 * after setting it results in the same value, because various mechanisms (like
 * the Actionscript Object.addProperty function or constant variables) can 
 * avoid this.
 **/
void
swfdec_as_object_set_variable_and_flags (SwfdecAsObject *object,
    const char *variable, const SwfdecAsValue *value, guint default_flags)
{
  SwfdecAsObjectClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (SWFDEC_IS_AS_VALUE (value));

  if (swfdec_gc_object_get_context (object)->debugger) {
    SwfdecAsDebugger *debugger = swfdec_gc_object_get_context (object)->debugger;
    SwfdecAsDebuggerClass *dklass = SWFDEC_AS_DEBUGGER_GET_CLASS (debugger);
    if (dklass->set_variable)
      dklass->set_variable (debugger, swfdec_gc_object_get_context (object), object, variable, value);
  }
  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  klass->set (object, variable, value, default_flags);
}

/**
 * swfdec_as_object_peek_variable:
 * @object: the object to query
 * @name: name of the variable to query
 *
 * Checks if the given @object contains a variable wih the given @name and if 
 * so, returns a pointer to its value. This pointer will be valid until calling
 * a setting function on the given object again.
 * <warning><para>This function is internal as it provides a pointer to an 
 * internal structure. Do not use it unless you are sure you need to. This
 * function skips prototypes, variables added via swfdec_as_value_add_variable()
 * and does not verify visibility flags.</para></warning>
 *
 * Returns: a pointer to the queried variable or %NULL if it doesn't exist
 **/
SwfdecAsValue *
swfdec_as_object_peek_variable (SwfdecAsObject *object, const char *name)
{
  SwfdecAsVariable *var;
  
  var = swfdec_as_object_hash_lookup (object, name);
  if (var == NULL ||
      var->get != NULL)
    return NULL;

  return &var->value;
}

/**
 * swfdec_as_object_get_variable:
 * @object: a #SwfdecAsObject
 * @variable: a garbage-collected string containing the name of the variable
 * @value: pointer to a #SwfdecAsValue that takes the return value or %NULL
 *
 * Gets the value of the given @variable on @object. It walks the prototype 
 * chain. This is a shortcut macro for 
 * swfdec_as_object_get_variable_and_flags().
 *
 * Returns: %TRUE if the variable existed, %FALSE otherwise
 */

/**
 * swfdec_as_object_get_variable_and_flags:
 * @object: a #SwfdecAsObject
 * @variable: a garbage-collected string containing the name of the variable
 * @value: pointer to a #SwfdecAsValue that takes the return value or %NULL
 * @flags: pointer to a guint taking the variable's flags or %NULL
 * @pobject: pointer to set to the object that really holds the property or 
 *           %NULL
 *
 * Looks up @variable on @object. It also walks the object's prototype chain.
 * If the variable exists, its value, flags and the real object containing the
 * variable will be set and %TRUE will be returned.
 *
 * Returns: %TRUE if the variable exists, %FALSE otherwise
 **/
gboolean
swfdec_as_object_get_variable_and_flags (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint *flags, SwfdecAsObject **pobject)
{
  SwfdecAsObjectClass *klass;
  guint i;
  SwfdecAsValue tmp_val;
  guint tmp_flags;
  SwfdecAsObject *tmp_pobject, *cur, *resolve;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (variable != NULL, FALSE);

  if (value == NULL)
    value = &tmp_val;
  if (flags == NULL)
    flags = &tmp_flags;
  if (pobject == NULL)
    pobject = &tmp_pobject;

  if (object->super) {
    cur = SWFDEC_AS_SUPER (object->relay)->object;
    if (cur)
      cur = cur->prototype;
  } else {
    cur = object;
  }

  resolve = NULL;
  for (i = 0; i <= SWFDEC_AS_OBJECT_PROTOTYPE_RECURSION_LIMIT && cur != NULL; i++) {
    klass = SWFDEC_AS_OBJECT_GET_CLASS (cur);
    if (klass->get (cur, object, variable, value, flags)) {
      *pobject = cur;
      return TRUE;
    }
    if (resolve == NULL) {
      SwfdecAsVariable *var =
	swfdec_as_object_hash_lookup (cur, SWFDEC_AS_STR___resolve);

      if (var != NULL && (swfdec_gc_object_get_context (object)->version <= 6 ||
	    SWFDEC_AS_VALUE_IS_COMPOSITE (&var->value)))
	resolve = cur;
    }
    cur = swfdec_as_object_get_prototype_internal (cur);
  }
  if (i > SWFDEC_AS_OBJECT_PROTOTYPE_RECURSION_LIMIT) {
    swfdec_as_context_abort (swfdec_gc_object_get_context (object), "Prototype recursion limit exceeded");
    SWFDEC_AS_VALUE_SET_UNDEFINED (value);
    *flags = 0;
    *pobject = NULL;
    return FALSE;
  }
  if (variable != SWFDEC_AS_STR___resolve && resolve != NULL) {
    SwfdecAsValue argv;
    SwfdecAsVariable *var;
    SwfdecAsFunction *fun;
    SwfdecAsContext *context;

    *flags = 0;
    *pobject = resolve;
    SWFDEC_AS_VALUE_SET_UNDEFINED (value);
    context = swfdec_gc_object_get_context (resolve);

    var = swfdec_as_object_hash_lookup (resolve, SWFDEC_AS_STR___resolve);
    g_assert (var != NULL);
    if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&var->value))
      return FALSE;
    fun = (SwfdecAsFunction *) (SWFDEC_AS_VALUE_GET_COMPOSITE (&var->value)->relay);
    if (!SWFDEC_IS_AS_FUNCTION (fun))
      return FALSE;
    SWFDEC_AS_VALUE_SET_STRING (&argv, variable);
    swfdec_as_function_call (fun, resolve, 1, &argv, value);

    return TRUE;
  }
  //SWFDEC_WARNING ("no such variable %s", variable);
  SWFDEC_AS_VALUE_SET_UNDEFINED (value);
  *flags = 0;
  *pobject = NULL;
  return FALSE;
}

/**
 * swfdec_as_object_has_variable:
 * @object: a #SwfdecAsObject
 * @variable: garbage-collected variable name
 *
 * Checks if a user-set @variable with the given name exists on @object. This 
 * function does not check variables that are available via an overwritten get 
 * function of the object's class.
 *
 * Returns: the object in the prototype chain that contains @variable or %NULL
 *          if the @object does not contain this variable.
 **/
SwfdecAsObject *
swfdec_as_object_has_variable (SwfdecAsObject *object, const char *variable)
{
  guint i;
  SwfdecAsVariable *var;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  
  for (i = 0; i <= SWFDEC_AS_OBJECT_PROTOTYPE_RECURSION_LIMIT && object != NULL; i++) {
    var = swfdec_as_object_hash_lookup (object, variable);
    if (var) {
      /* FIXME: propflags? */
      return object;
    }
    object = swfdec_as_object_get_prototype_internal (object);
  }
  return NULL;
}

/**
 * swfdec_as_object_delete_variable:
 * @object: a #SwfdecAsObject
 * @variable: garbage-collected name of the variable
 *
 * Deletes the given variable if possible. If the variable is protected from 
 * deletion, it will not be deleted.
 *
 * Returns: See #SwfdecAsDeleteReturn for details of the return value.
 **/
SwfdecAsDeleteReturn
swfdec_as_object_delete_variable (SwfdecAsObject *object, const char *variable)
{
  SwfdecAsVariable *var;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (variable != NULL, FALSE);

  var = g_hash_table_lookup (object->properties, variable);
  if (var == NULL)
    return SWFDEC_AS_DELETE_NOT_FOUND;
  if (var->flags & SWFDEC_AS_VARIABLE_PERMANENT)
    return SWFDEC_AS_DELETE_NOT_DELETED;

  // Note: We won't remove object->prototype, even if __proto__ is deleted

  swfdec_as_object_free_property (NULL, var, object);
  if (!g_hash_table_remove (object->properties, variable)) {
    g_assert_not_reached ();
  }
  return SWFDEC_AS_DELETE_DELETED;
}

/**
 * swfdec_as_object_delete_all_variables:
 * @object: a #SwfdecAsObject
 *
 * Deletes all user-set variables from the given object.
 **/
void
swfdec_as_object_delete_all_variables (SwfdecAsObject *object)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));

  g_hash_table_foreach (object->properties, swfdec_as_object_free_property, object);
  g_hash_table_remove_all (object->properties);
}

/**
 * swfdec_as_object_set_variable_flags:
 * @object: a #SwfdecAsObject
 * @variable: the variable to modify
 * @flags: flags to set
 *
 * Sets the given flags for the given variable.
 **/
void
swfdec_as_object_set_variable_flags (SwfdecAsObject *object, 
    const char *variable, SwfdecAsVariableFlag flags)
{
  SwfdecAsVariable *var;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);

  var = swfdec_as_object_hash_lookup (object, variable);
  if (var == NULL)
    return;

  var->flags |= flags;

  if (variable == SWFDEC_AS_STR___proto__)
    object->prototype_flags = var->flags;
}

/**
 * swfdec_as_object_unset_variable_flags:
 * @object: a #SwfdecAsObject
 * @variable: the variable to modify
 * @flags: flags to unset
 *
 * Unsets the given flags for the given variable. The variable must exist in 
 * @object.
 **/
void
swfdec_as_object_unset_variable_flags (SwfdecAsObject *object,
    const char *variable, SwfdecAsVariableFlag flags)
{
  SwfdecAsVariable *var;
  

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);

  var = swfdec_as_object_hash_lookup (object, variable);
  if (var == NULL)
    return;

  var->flags &= ~flags;

  if (variable == SWFDEC_AS_STR___proto__)
    object->prototype_flags = var->flags;
}

/**
 * swfdec_as_object_foreach:
 * @object: a #SwfdecAsObject
 * @func: function to call
 * @data: data to pass to @func
 *
 * Calls @func for every variable of @object or until @func returns %FALSE. The
 * variables of @object must not be modified by @func.
 *
 * Returns: %TRUE if @func always returned %TRUE
 **/
gboolean
swfdec_as_object_foreach (SwfdecAsObject *object, SwfdecAsVariableForeach func,
    gpointer data)
{
  SwfdecAsObjectClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  g_return_val_if_fail (klass->foreach != NULL, FALSE);
  return klass->foreach (object, func, data);
}

/*** SIMPLIFICATIONS ***/

static void
swfdec_as_object_do_nothing (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
}

/**
 * swfdec_as_object_add_function:
 * @object: a #SwfdecAsObject
 * @name: name of the function. The string does not have to be 
 *        garbage-collected.
 * @native: a native function or %NULL to just not do anything
 *
 * Adds @native as a variable named @name to @object. The newly added variable
 * will not be enumerated.
 *
 * Returns: the newly created #SwfdecAsFunction
 **/
SwfdecAsFunction *
swfdec_as_object_add_function (SwfdecAsObject *object, const char *name, SwfdecAsNative native)
{
  SwfdecAsFunction *function;
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  cx = swfdec_gc_object_get_context (object);
  if (!native)
    native = swfdec_as_object_do_nothing;
  function = swfdec_as_native_function_new (cx, name, native, NULL);

  name = swfdec_as_context_get_string (cx, name);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (function)));
  /* FIXME: I'd like to make sure no such property exists yet */
  swfdec_as_object_set_variable_and_flags (object, name, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

  return function;
}

/**
 * swfdec_as_object_run:
 * @object: a #SwfdecAsObject
 * @script: script to execute
 *
 * Executes the given @script with @object as this pointer.
 **/
void
swfdec_as_object_run (SwfdecAsObject *object, SwfdecScript *script)
{
  SwfdecAsFrame frame = { NULL, };
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (script != NULL);

  context = swfdec_gc_object_get_context (object);
  swfdec_as_frame_init (&frame, context, script);
  swfdec_as_frame_set_this (&frame, object);
  swfdec_as_frame_preload (&frame);
  /* we take no prisoners */
  frame.activation = NULL;
  swfdec_as_context_run (context);
  swfdec_as_stack_pop (context);
}

/**
 * swfdec_as_object_call:
 * @object: a #SwfdecAsObject
 * @name: garbage-collected string naming the function to call. 
 * @argc: number of arguments to provide to function
 * @argv: arguments or %NULL when @argc is 0
 * @return_value: location to take the return value of the call or %NULL to 
 *                ignore the return value.
 *
 * Calls the function named @name on the given object. This function is 
 * essentially equal to the folloeing Actionscript code: 
 * <informalexample><programlisting>
 * @return_value = @object.@name (@argv[0], ..., @argv[argc-1]);
 * </programlisting></informalexample>
 *
 * Returns: %TRUE if @object had a function with the given name, %FALSE otherwise
 **/
gboolean
swfdec_as_object_call (SwfdecAsObject *object, const char *name, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  SwfdecAsValue tmp;
  SwfdecAsFunction *fun;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), TRUE);
  g_return_val_if_fail (name != NULL, TRUE);
  g_return_val_if_fail (argc == 0 || argv != NULL, TRUE);
  g_return_val_if_fail (swfdec_gc_object_get_context (object)->global != NULL, TRUE); /* for SwfdecPlayer */

  if (return_value)
    SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);
  swfdec_as_object_get_variable (object, name, &tmp);
  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&tmp))
    return FALSE;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_COMPOSITE (&tmp)->relay;
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    return FALSE;
  swfdec_as_function_call (fun, object, argc, argv, return_value ? return_value : &tmp);

  return TRUE;
}

/**
 * swfdec_as_object_create:
 * @fun: constructor
 * @n_args: number of arguments
 * @args: arguments to pass to constructor
 * @return_value: pointer for return value or %NULL to push the return value to 
 *                the stack
 *
 * Creates a new object for the given constructor and runs the constructor.
 **/
void
swfdec_as_object_create (SwfdecAsFunction *fun, guint n_args, 
    const SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsValue val;
  SwfdecAsObject *new, *fun_object;
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (fun));

  context = swfdec_gc_object_get_context (fun);
  fun_object = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (fun));

  new = g_object_new (SWFDEC_TYPE_AS_OBJECT, "context", context, NULL);
  /* set initial variables */
  if (swfdec_as_object_get_variable (fun_object, SWFDEC_AS_STR_prototype, &val)) {
      swfdec_as_object_set_variable_and_flags (new, SWFDEC_AS_STR___proto__,
	  &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  }
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, fun_object);
  if (context->version < 7) {
    swfdec_as_object_set_variable_and_flags (new, SWFDEC_AS_STR_constructor, 
	&val, SWFDEC_AS_VARIABLE_HIDDEN);
  }
  swfdec_as_object_set_variable_and_flags (new, SWFDEC_AS_STR___constructor__, 
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_VERSION_6_UP);

  swfdec_as_function_call_full (fun, new, TRUE, new->prototype, n_args, args, return_value);
}

SwfdecAsObject *
swfdec_as_object_set_constructor_by_name (SwfdecAsObject *object, const char *name, ...)
{
  SwfdecAsObject *ret;
  va_list args;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  va_start (args, name);
  ret = swfdec_as_object_set_constructor_by_namev (object, name, args);
  va_end (args);
  return ret;
}

SwfdecAsObject *
swfdec_as_object_set_constructor_by_namev (SwfdecAsObject *object, 
    const char *name, va_list args)
{
  SwfdecAsContext *context;
  SwfdecAsObject *cur;
  SwfdecAsValue *val;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  context = swfdec_gc_object_get_context (object);
  cur = context->global;
  do {
    val = swfdec_as_object_peek_variable (cur, name);
    if (val == NULL ||
	!SWFDEC_AS_VALUE_IS_COMPOSITE (val)) {
      SWFDEC_WARNING ("could not find constructor %s", name);
      return NULL;
    }
    cur = SWFDEC_AS_VALUE_GET_COMPOSITE (val);
    name = va_arg (args, const char *);
  } while (name != NULL);
  swfdec_as_object_set_constructor (object, cur);
  return cur;
}

/**
 * swfdec_as_object_set_constructor:
 * @object: a #SwfdecAsObject
 * @construct: the constructor of @object
 *
 * Sets the constructor variables for @object. Most objects get these 
 * variables set automatically, but for objects you created yourself, you want
 * to call this function. This is essentially the same as the following script
 * code:
 * |[ object.constructor = construct;
 * object.__proto__ = construct.prototype; ]|
 **/
void
swfdec_as_object_set_constructor (SwfdecAsObject *object, SwfdecAsObject *construct)
{
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (construct));

  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, construct);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_constructor, 
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  if (swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (construct),
	  SWFDEC_AS_STR_prototype, &val)) {
    swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR___proto__, 
	&val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  } else {
    SWFDEC_WARNING ("constructor has no prototype, not setting any");
  }
}

/**
 * swfdec_as_object_add_variable:
 * @object: a #SwfdecAsObject
 * @variable: name of the variable
 * @get: getter function to call when reading the variable
 * @set: setter function to call when writing the variable or %NULL if read-only
 * @default_flags: flags to use if creating the variable anew - the flags will
 *                 be ignored if the property already exists.
 *
 * Adds a variable to @object in the same way as the Actionscript code 
 * "object.addProperty()" would do. Accessing the variable will from now on be
 * handled by calling the @get or @set functions. A previous value of the 
 * variable or a previous call to this function will be overwritten.
 **/
void
swfdec_as_object_add_variable (SwfdecAsObject *object, const char *variable, 
    SwfdecAsFunction *get, SwfdecAsFunction *set, guint default_flags)
{
  SwfdecAsVariable *var;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (get));
  g_return_if_fail (set == NULL || SWFDEC_IS_AS_FUNCTION (set));

  var = swfdec_as_object_hash_lookup (object, variable);
  if (var == NULL)
    var = swfdec_as_object_hash_create (object, variable, default_flags);
  if (var == NULL)
    return;
  var->get = get;
  var->set = set;
}

void
swfdec_as_object_add_native_variable (SwfdecAsObject *object,
    const char *variable, SwfdecAsNative get, SwfdecAsNative set)
{
  SwfdecAsFunction *get_func, *set_func;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (get != NULL);

  get_func =
    swfdec_as_native_function_new (swfdec_gc_object_get_context (object), variable, get, NULL);

  if (set != NULL) {
    set_func =
      swfdec_as_native_function_new (swfdec_gc_object_get_context (object), variable, set, NULL);
  } else {
    set_func = NULL;
  }

  swfdec_as_object_add_variable (object, variable, get_func, set_func, 0);
}

/*** AS CODE ***/

SWFDEC_AS_NATIVE (101, 2, swfdec_as_object_addProperty)
void
swfdec_as_object_addProperty (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecAsFunction *get, *set;
  const char *name;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);
  if (argc < 3)
    return;
  name = swfdec_as_value_to_string (cx, &argv[0]);
  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&argv[1]) ||
      !SWFDEC_IS_AS_FUNCTION ((get = (SwfdecAsFunction *) (SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[1])->relay))))
    return;
  if (SWFDEC_AS_VALUE_IS_COMPOSITE (&argv[2])) {
    set = (SwfdecAsFunction *) (SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[2])->relay);
    if (!SWFDEC_IS_AS_FUNCTION (set))
      return;
  } else if (SWFDEC_AS_VALUE_IS_NULL (&argv[2])) {
    set = NULL;
  } else {
    return;
  }

  swfdec_as_object_add_variable (object, name, get, set, 0);
  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
}

SWFDEC_AS_NATIVE (101, 5, swfdec_as_object_hasOwnProperty)
void
swfdec_as_object_hasOwnProperty (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecAsVariable *var;
  const char *name;

  if (object == NULL)
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);

  // return false even if no params
  if (argc < 1)
    return;

  name = swfdec_as_value_to_string (swfdec_gc_object_get_context (object), &argv[0]);

  if (!(var = swfdec_as_object_hash_lookup (object, name)))
    return;

  /* This functions only checks NOT 6 flag, and checks it on ALL VERSIONS */
  if (var->flags & SWFDEC_AS_VARIABLE_VERSION_NOT_6)
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
}

SWFDEC_AS_NATIVE (101, 7, swfdec_as_object_isPropertyEnumerable)
void
swfdec_as_object_isPropertyEnumerable (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecAsVariable *var;
  const char *name;

  if (object == NULL)
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);

  // return false even if no params
  if (argc < 1)
    return;

  name = swfdec_as_value_to_string (swfdec_gc_object_get_context (object), &argv[0]);

  if (!(var = swfdec_as_object_hash_lookup (object, name)))
    return;

  if (var->flags & SWFDEC_AS_VARIABLE_HIDDEN)
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
}

SWFDEC_AS_NATIVE (101, 6, swfdec_as_object_isPrototypeOf)
void
swfdec_as_object_isPrototypeOf (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecAsObject *class;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);

  // return false even if no params
  if (argc < 1)
    return;

  class = swfdec_as_value_to_object (cx, &argv[0]);
  if (class == NULL)
    return;

  while ((class = swfdec_as_object_get_prototype (class)) != NULL) {
    if (object == class) {
      SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
      return;
    }
  }

  // not found, nothing to do
}

SWFDEC_AS_NATIVE (101, 0, swfdec_as_object_watch)
void
swfdec_as_object_watch (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecAsWatch *watch;
  const char *name;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);

  if (argc < 2)
    return;

  name = swfdec_as_value_to_string (cx, &argv[0]);

  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&argv[1]))
    return;

  if (!SWFDEC_IS_AS_FUNCTION (SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[1])->relay))
    return;

  if (object->watches == NULL) {
    object->watches = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
	NULL, (GDestroyNotify) swfdec_as_watch_unref);
    watch = NULL;
  } else {
    watch = g_hash_table_lookup (object->watches, name);
  }
  if (watch == NULL) {
    watch = swfdec_as_watch_new (SWFDEC_AS_FUNCTION (SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[1])->relay));
    if (watch == NULL)
      return;
    g_hash_table_insert (object->watches, (char *) name, watch);
  } else {
    watch->watch = SWFDEC_AS_FUNCTION (SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[1])->relay);
  }

  if (argc >= 3) {
    watch->watch_data = argv[2];
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&watch->watch_data);
  }

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
}

SWFDEC_AS_NATIVE (101, 1, swfdec_as_object_unwatch)
void
swfdec_as_object_unwatch (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecAsVariable *var;
  const char *name;

  if (object == NULL)
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);

  if (argc < 1)
    return;

  name = swfdec_as_value_to_string (cx, &argv[0]);

  // special case: can't unwatch native properties
  if ((var = swfdec_as_object_hash_lookup (object, name))&& var->get != NULL)
      return;

  if (object->watches != NULL && 
      g_hash_table_remove (object->watches, name)) {

    SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);

    if (g_hash_table_size (object->watches) == 0) {
      g_hash_table_destroy (object->watches);
      object->watches = NULL;
    }
  }
}

SWFDEC_AS_NATIVE (101, 3, swfdec_as_object_valueOf)
void
swfdec_as_object_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  if (object != NULL)
    SWFDEC_AS_VALUE_SET_COMPOSITE (retval, object);
}

SWFDEC_AS_NATIVE (101, 4, swfdec_as_object_toString)
void
swfdec_as_object_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  if (object && SWFDEC_IS_AS_FUNCTION (object->relay)) {
    SWFDEC_AS_VALUE_SET_STRING (retval, SWFDEC_AS_STR__type_Function_);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (retval, SWFDEC_AS_STR__object_Object_);
  }
}

// only available as ASnative
SWFDEC_AS_NATIVE (3, 3, swfdec_as_object_old_constructor)
void
swfdec_as_object_old_constructor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("old 'Object' function (only available as ASnative)");
}

void
swfdec_as_object_decode (SwfdecAsObject *object, const char *str)
{
  SwfdecAsContext *cx = swfdec_gc_object_get_context (object);
  SwfdecAsValue val;
  char **varlist, *p, *unescaped;
  guint i;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (str != NULL);

  varlist = g_strsplit (str, "&", -1);

  for (i = 0; varlist[i] != NULL; i++) {
    p = strchr (varlist[i], '=');
    if (p != NULL) {
      *p++ = '\0';
      if (*p == '\0')
	p = NULL;
    }

    if (p != NULL) {
      unescaped = swfdec_as_string_unescape (cx, p);
      if (unescaped != NULL) {
	SWFDEC_AS_VALUE_SET_STRING (&val,
	    swfdec_as_context_give_string (cx, unescaped));
      } else {
	SWFDEC_AS_VALUE_SET_STRING (&val, SWFDEC_AS_STR_EMPTY);
      }
    } else {
      SWFDEC_AS_VALUE_SET_STRING (&val, SWFDEC_AS_STR_EMPTY);
    }
    unescaped = swfdec_as_string_unescape (cx, varlist[i]);
    if (unescaped != NULL) {
      swfdec_as_object_set_variable (object,
	  swfdec_as_context_give_string (cx, unescaped), &val);
    }
  }
  g_strfreev (varlist);
}

static void
swfdec_as_object_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (argc > 0) {
    SwfdecAsObject *result = swfdec_as_value_to_object (cx, &argv[0]);
    if (result != NULL) {
      if (!cx->frame->construct) {
	SWFDEC_AS_VALUE_SET_COMPOSITE (ret, result);
      } else {
	SWFDEC_FIXME ("new Object (x) should return x");
	SWFDEC_AS_VALUE_SET_COMPOSITE (ret, object);
      }
      return;
    }
  }

  if (!cx->frame->construct)
    object = swfdec_as_object_new_empty (cx);

  SWFDEC_AS_VALUE_SET_COMPOSITE (ret, object);
}

void
swfdec_as_object_init_context (SwfdecAsContext *context)
{
  SwfdecAsObject *function, *fun_proto, *object, *obj_proto;
  SwfdecAsFunction *fun;
  SwfdecAsValue val;

  /* initialize core objects */
  fun = swfdec_as_native_function_new_bare (context, SWFDEC_AS_STR_Object, swfdec_as_object_construct, NULL);
  object = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (fun));
  obj_proto = swfdec_as_object_new_empty (context);
  fun = swfdec_as_native_function_new_bare (context, SWFDEC_AS_STR_Function, swfdec_as_object_do_nothing, NULL);
  function = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (fun));
  fun_proto = swfdec_as_object_new_empty (context);

  /* initialize Function */
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, function);
  swfdec_as_object_set_variable_and_flags (context->global, SWFDEC_AS_STR_Function, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT | SWFDEC_AS_VARIABLE_VERSION_6_UP);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, function);
  swfdec_as_object_set_variable_and_flags (function, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, fun_proto);
  swfdec_as_object_set_variable_and_flags (function, SWFDEC_AS_STR_prototype,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  swfdec_as_object_set_variable_and_flags (function, SWFDEC_AS_STR___proto__,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT |
      SWFDEC_AS_VARIABLE_VERSION_6_UP);

  /* initialize Function.prototype */
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, function);
  swfdec_as_object_set_variable_and_flags (fun_proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, obj_proto);
  swfdec_as_object_set_variable_and_flags (fun_proto,
      SWFDEC_AS_STR___proto__, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);


  /* initialize Object */
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, object);
  swfdec_as_object_set_variable_and_flags (context->global, SWFDEC_AS_STR_Object, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, function);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, fun_proto);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR___proto__,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT |
      SWFDEC_AS_VARIABLE_VERSION_6_UP);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, obj_proto);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_prototype,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT |
      SWFDEC_AS_VARIABLE_CONSTANT);

  /* initialize Object.prototype */
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, object);
  swfdec_as_object_set_variable_and_flags (obj_proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
}

/**
 * swfdec_as_object_resolve:
 * @object: a #SwfdecAsObject
 *
 * Resolves the object to its real object. Some internal objects should not be
 * exposed to scripts, for example #SwfdecAsFrame objects. If an object you want
 * to expose might be internal, call this function to resolve it to an object
 * that is safe to expose.
 *
 * Returns: a non-internal object
 **/
SwfdecAsObject *
swfdec_as_object_resolve (SwfdecAsObject *object)
{
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);

  if (G_UNLIKELY (object->super))
    return SWFDEC_AS_SUPER (object->relay)->thisp;
  
  return object;
}

void
swfdec_as_object_set_relay (SwfdecAsObject *object, SwfdecAsRelay *relay)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  if (relay) {
    g_return_if_fail (SWFDEC_IS_AS_RELAY (relay));
    g_return_if_fail (relay->relay == NULL);
  }

  if (object->relay) {
    object->relay->relay = NULL;
  }
  object->relay = relay;
  object->array = FALSE;
  if (relay)
    relay->relay = object;
}

