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

#include "swfdec_as_function.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_stack.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsFunction, swfdec_as_function, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_function_dispose (GObject *object)
{
  SwfdecAsFunction *function = SWFDEC_AS_FUNCTION (object);

  if (function->script) {
    swfdec_script_unref (function->script);
    function->script = NULL;
  }
  g_free (function->name);
  function->name = NULL;

  G_OBJECT_CLASS (swfdec_as_function_parent_class)->dispose (object);
}

static void
swfdec_as_function_mark (SwfdecAsObject *object)
{
  SwfdecAsFunction *function = SWFDEC_AS_FUNCTION (object);

  if (function->scope)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (function->scope));

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_function_parent_class)->mark (object);
}

static void
swfdec_as_function_class_init (SwfdecAsFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_function_dispose;

  asobject_class->mark = swfdec_as_function_mark;
}

static void
swfdec_as_function_init (SwfdecAsFunction *function)
{
  function->type = SWFDEC_TYPE_AS_OBJECT;
  function->type_size = sizeof (SwfdecAsObject);
}

SwfdecAsFunction *
swfdec_as_function_do_create (SwfdecAsContext *context)
{
  SwfdecAsValue val;
  SwfdecAsObject *fun;

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsFunction)))
    return NULL;
  fun = g_object_new (SWFDEC_TYPE_AS_FUNCTION, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (fun), context, sizeof (SwfdecAsFunction));
  swfdec_as_object_root (fun);
  if (context->Function) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
    swfdec_as_object_set_variable (fun, SWFDEC_AS_STR_constructor, &val);
  }
  if (context->Function_prototype) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function_prototype);
    swfdec_as_object_set_variable (fun, SWFDEC_AS_STR___proto__, &val);
  }
  swfdec_as_object_unroot (fun);

  return SWFDEC_AS_FUNCTION (fun);
}

SwfdecAsFunction *
swfdec_as_function_new (SwfdecAsScope *scope)
{
  SwfdecAsValue val;
  SwfdecAsFunction *fun;
  SwfdecAsObject *proto;

  g_return_val_if_fail (SWFDEC_IS_AS_SCOPE (scope), NULL);

  fun = swfdec_as_function_do_create (SWFDEC_AS_OBJECT (scope)->context);
  if (fun == NULL)
    return NULL;
  proto = swfdec_as_object_new (SWFDEC_AS_OBJECT (scope)->context);
  if (proto == NULL)
    return NULL;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (fun));
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (fun), SWFDEC_AS_STR_prototype, &val);
  fun->scope = scope;

  return fun;
}

SwfdecAsFunction *
swfdec_as_function_new_native (SwfdecAsContext *context, const char *name,
    SwfdecAsNative native, guint min_args)
{
  SwfdecAsFunction *fun;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (native != NULL, NULL);

  fun = swfdec_as_function_do_create (context);
  if (fun == NULL)
    return NULL;
  fun->native = native;
  fun->min_args = min_args;
  fun->name = g_strdup (name);

  return fun;
}

void
swfdec_as_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, guint n_args,
    SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (function));
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (thisp));
  g_return_if_fail (n_args == 0 || args != NULL);
  g_return_if_fail (return_value != NULL);

  context = thisp->context;
  SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);
  if (function->native) {
    frame = swfdec_as_frame_new_native (thisp);
    g_assert (function->name);
    frame->function_name = function->name;
  } else {
    frame = swfdec_as_frame_new (thisp, function->script);
    SWFDEC_AS_SCOPE (frame)->next = function->scope;
    frame->scope = SWFDEC_AS_SCOPE (frame);
  }
  frame->var_object = SWFDEC_AS_OBJECT (frame);
  frame->argc = n_args;
  frame->argv = args;
  frame->return_value = return_value;
  frame->function = function;
  swfdec_as_frame_preload (frame);
}

/**
 * swfdec_as_function_set_object_type:
 * @function: a native #SwfdecAsFunction
 * @type: required #GType for this object
 *
 * Sets the required type for the this object to @type. If the this object 
 * isn't of the required type, the function will not be called and its
 * return value will be undefined.
 **/
void
swfdec_as_function_set_object_type (SwfdecAsFunction *function, GType type)
{
  GTypeQuery query;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (function));
  g_return_if_fail (g_type_is_a (type, SWFDEC_TYPE_AS_OBJECT));

  g_type_query (type, &query);
  function->type = type;
  function->type_size = query.instance_size;
}

/*** AS CODE ***/

static void
swfdec_as_function_construct (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{

}

void
swfdec_as_function_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *function, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  function = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_Function, 0, swfdec_as_function_construct, 0));
  if (!function)
    return;
  context->Function = function;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, function);
  swfdec_as_object_set_variable (function, SWFDEC_AS_STR_constructor, &val);
  if (version > 5) {
    proto = swfdec_as_object_new (context);
    if (!proto)
      return;
    context->Function_prototype = proto;
    SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
    swfdec_as_object_set_variable (function, SWFDEC_AS_STR___proto__, &val);
    swfdec_as_object_set_variable (function, SWFDEC_AS_STR_prototype, &val);
  }
}

