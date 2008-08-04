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
#include "swfdec_as_super.h"
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
  context = swfdec_gc_object_get_context (fun);
  if (context->Function == NULL)
    return;
  
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function_prototype);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR___proto__,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT |
      SWFDEC_AS_VARIABLE_VERSION_6_UP);
}

SwfdecAsFrame *
swfdec_as_function_call_no_preload (SwfdecAsFunction *function, 
    SwfdecAsObject *thisp, guint n_args, const SwfdecAsValue *args, 
    SwfdecAsValue *return_value)
{
  SwfdecAsFrame *frame;
  SwfdecAsFunctionClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_FUNCTION (function), NULL);
  g_return_val_if_fail (thisp == NULL || SWFDEC_IS_AS_OBJECT (thisp), NULL);

  /* just to be sure... */
  if (return_value)
    SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);

  klass = SWFDEC_AS_FUNCTION_GET_CLASS (function);
  g_assert (klass->old_call);
  frame = klass->old_call (function);
  /* FIXME: figure out what to do in these situations?
   * It's a problem when called inside swfdec_as_function_call () as the
   * user of that function expects success, but super may fail here */
  if (frame == NULL)
    return NULL;
  /* second check especially for super object */
  if (thisp != NULL && frame->thisp == NULL) {
    swfdec_as_frame_set_this (frame, swfdec_as_object_resolve (thisp));
  }
  frame->argc = n_args;
  frame->argv = args;
  frame->return_value = return_value;
  return frame;
}

/**
 * swfdec_as_function_call:
 * @function: the #SwfdecAsFunction to call
 * @thisp: this argument to use for the call or %NULL for none
 * @n_args: number of arguments to pass to the function
 * @args: the arguments to pass or %NULL to read the last @n_args stack elements.
 *        The memory must be unchanged until the function call has completed.
 *        This is after the call to swfdec_as_context_run () has finished.
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
  SwfdecAsFrame *frame;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (function));
  g_return_if_fail (thisp == NULL || SWFDEC_IS_AS_OBJECT (thisp));

  frame = swfdec_as_function_call_no_preload (function, thisp, n_args, args, return_value);
  if (frame == NULL)
    return;
  if (thisp != NULL) {
    swfdec_as_super_new (frame, thisp, thisp->prototype);
  } else {
    // FIXME: Does the super object really reference the function when thisp is NULL?
    swfdec_as_super_new (frame, SWFDEC_AS_OBJECT (function), SWFDEC_AS_OBJECT (function)->prototype);
  }
  swfdec_as_frame_preload (frame);
}

/*** AS CODE ***/

SWFDEC_AS_NATIVE (101, 10, swfdec_as_function_do_call)
void
swfdec_as_function_do_call (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsFunction *fun;
  SwfdecAsObject *thisp = NULL;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_FUNCTION, &fun, "|O", &thisp);

  if (thisp == NULL) {
    thisp = swfdec_as_object_new_empty (cx);
  }
  if (argc > 0) {
    argc--;
    argv++;
  }
  swfdec_as_function_call (fun, thisp, argc, argv, ret);
  swfdec_as_context_run (cx);
}

SWFDEC_AS_NATIVE (101, 11, swfdec_as_function_apply)
void
swfdec_as_function_apply (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsValue *argv_pass = NULL;
  int length = 0;
  SwfdecAsFunction *fun;
  SwfdecAsObject *thisp = NULL;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_FUNCTION, &fun, "|O", &thisp);

  if (thisp == NULL)
    thisp = swfdec_as_object_new_empty (cx);

  if (argc > 1 && SWFDEC_AS_VALUE_IS_OBJECT (&argv[1])) {
    int i;
    SwfdecAsObject *array;
    SwfdecAsValue val;

    array = SWFDEC_AS_VALUE_GET_OBJECT (&argv[1]);

    swfdec_as_object_get_variable (array, SWFDEC_AS_STR_length, &val);
    length = swfdec_as_value_to_integer (cx, &val);

    if (length > 0) {
      /* FIXME: find a smarter way to do this, like providing argv not as an array */
      if (!swfdec_as_context_try_use_mem (cx, sizeof (SwfdecAsValue) * length)) {
	swfdec_as_context_abort (cx, "too many arguments to Function.apply");
	return;
      }
      argv_pass = g_malloc (sizeof (SwfdecAsValue) * length);

      for (i = 0; i < length; i++) {
	swfdec_as_object_get_variable (array,
	    swfdec_as_integer_to_string (cx, i), &argv_pass[i]);
      }
    } else {
      length = 0;
    }
  }

  swfdec_as_function_call (fun, thisp, length, argv_pass, ret);
  swfdec_as_context_run (cx);

  if (argv_pass) {
    swfdec_as_context_unuse_mem (cx, sizeof (SwfdecAsValue) * length);
    g_free (argv_pass);
  }
}

void
swfdec_as_function_init_context (SwfdecAsContext *context)
{
  SwfdecAsObject *function, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  function = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_Function, NULL));
  swfdec_as_object_set_variable_flags (context->global, SWFDEC_AS_STR_Function, SWFDEC_AS_VARIABLE_VERSION_6_UP);
  context->Function = function;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, function);
  swfdec_as_object_set_variable_and_flags (function, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  proto = swfdec_as_object_new_empty (context);
  context->Function_prototype = proto;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable_and_flags (function, SWFDEC_AS_STR_prototype,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  swfdec_as_object_set_variable_and_flags (function, SWFDEC_AS_STR___proto__,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT |
      SWFDEC_AS_VARIABLE_VERSION_6_UP);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, function);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
}

