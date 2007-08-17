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
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

G_DEFINE_ABSTRACT_TYPE (SwfdecAsFunction, swfdec_as_function, SWFDEC_TYPE_AS_OBJECT)

/**
 * SECTION:SwfdecAsFunction
 * @title: SwfdecAsFunction
 * @short_description: script objects that can be executed
 *
 * Functions is the basic object for executing code in the Swfdec script engine.
 * There is multiple different variants of functions, such as script-created 
 * ones and native functions.
 *
 * If you want to create your own functions, you should create native functions.
 * The easiest way to do this is with swfdec_as_object_add_function() or
 * swfdec_as_native_function_new().
 *
 * In Actionscript, every function can be used as a constructor. If you want to
 * make a native function be used as a constructor for your own #SwfdecAsObject
 * subclass, have a look at swfdec_as_native_function_set_construct_type().
 */

/**
 * SwfdecAsFunction
 *
 * This is the base executable object in Swfdec. It is an abstract object. If 
 * you want to create functions yourself, use #SwfdecAsNativeFunction.
 */

static void
swfdec_as_function_class_init (SwfdecAsFunctionClass *klass)
{
}

static void
swfdec_as_function_init (SwfdecAsFunction *function)
{
}

/**
 * swfdec_as_function_set_constructor:
 * @fun: a #SwfdecAsFunction
 *
 * Sets the constructor and prototype of @fun. This is a shortcut for calling
 * swfdec_as_object_set_constructor() with the right arguments.
 **/
void
swfdec_as_function_set_constructor (SwfdecAsFunction *fun)
{
  SwfdecAsContext *context;
  SwfdecAsObject *object;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (fun));

  object = SWFDEC_AS_OBJECT (fun);
  context = object->context;
  if (context->Function) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_constructor, &val);
  }
  if (context->Function_prototype) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function_prototype);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR___proto__, &val);
  }
}

/**
 * swfdec_as_function_call:
 * @function: the #SwfdecAsFunction to call
 * @thisp: this argument to use for the call or %NULL for none
 * @n_args: number of arguments to pass to the function
 * @args: the arguments to pass or %NULL to read the last @n_args stack elements
 * @return_value: pointer for return value or %NULL to push the return value to 
 *                the stack
 *
 * Calls the given function. This means a #SwfdecAsFrame is created for the 
 * function and pushed on top of the execution stack. The function is however
 * not executed. Call swfdec_as_context_run () to execute it.
 **/
void
swfdec_as_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, guint n_args,
    const SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;
  SwfdecAsFunctionClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (function));
  g_return_if_fail (thisp == NULL || SWFDEC_IS_AS_OBJECT (thisp));

  context = SWFDEC_AS_OBJECT (function)->context;
  /* just to be sure... */
  if (return_value)
    SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);

  klass = SWFDEC_AS_FUNCTION_GET_CLASS (function);
  g_assert (klass->call);
  frame = klass->call (function);
  /* FIXME: figure out what to do in these situations */
  if (frame == NULL)
    return;
  /* second check especially for super object */
  if (thisp != NULL && frame->thisp == NULL)
    swfdec_as_frame_set_this (frame, swfdec_as_object_resolve (thisp));
  frame->is_local = TRUE;
  frame->argc = n_args;
  frame->argv = args;
  frame->return_value = return_value;
  swfdec_as_frame_preload (frame);
}

/*** AS CODE ***/

static void
swfdec_as_function_do_call (SwfdecAsContext *context, SwfdecAsObject *fun,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *thisp;

  if (argc > 0) {
    thisp = swfdec_as_value_to_object (context, &argv[0]);
    argc -= 1;
    argv++;
  } else {
    thisp = NULL;
  }
  if (thisp == NULL)
    thisp = swfdec_as_object_new_empty (context);
  swfdec_as_function_call (SWFDEC_AS_FUNCTION (fun), thisp, argc, argv, ret);
  swfdec_as_context_run (context);
}

void
swfdec_as_function_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *function, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  function = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_Function, 0, NULL, 0));
  if (!function)
    return;
  if (version < 6) {
    /* deleting it later on is easier than duplicating swfdec_as_object_add_function() */
    swfdec_as_object_delete_variable (context->global, SWFDEC_AS_STR_Function);
  }
  context->Function = function;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, function);
  swfdec_as_object_set_variable (function, SWFDEC_AS_STR_constructor, &val);
  if (version > 5) {
    proto = swfdec_as_object_new_empty (context);
    if (!proto)
      return;
    context->Function_prototype = proto;
    SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
    swfdec_as_object_set_variable (function, SWFDEC_AS_STR___proto__, &val);
    swfdec_as_object_set_variable (function, SWFDEC_AS_STR_prototype, &val);
    /* prototype functions */
    swfdec_as_object_add_function (proto, SWFDEC_AS_STR_call, SWFDEC_TYPE_AS_FUNCTION, 
	swfdec_as_function_do_call, 0);
  }
}

