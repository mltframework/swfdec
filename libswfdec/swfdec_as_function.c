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

G_DEFINE_ABSTRACT_TYPE (SwfdecAsFunction, swfdec_as_function, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_function_class_init (SwfdecAsFunctionClass *klass)
{
}

static void
swfdec_as_function_init (SwfdecAsFunction *function)
{
}

SwfdecAsFunction *
swfdec_as_function_create (SwfdecAsContext *context, GType type, guint size)
{
  SwfdecAsValue val;
  SwfdecAsObject *fun;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (g_type_is_a (type, SWFDEC_TYPE_AS_FUNCTION), NULL);
  g_return_val_if_fail (size >= sizeof (SwfdecAsFunction), NULL);

  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  fun = g_object_new (type, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (fun), context, size);
  if (context->Function) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
    swfdec_as_object_set_variable (fun, SWFDEC_AS_STR_constructor, &val);
  }
  if (context->Function_prototype) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function_prototype);
    swfdec_as_object_set_variable (fun, SWFDEC_AS_STR___proto__, &val);
  }

  return SWFDEC_AS_FUNCTION (fun);
}

void
swfdec_as_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, guint n_args,
    SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;
  SwfdecAsFunctionClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (function));
  g_return_if_fail (thisp == NULL || SWFDEC_IS_AS_OBJECT (thisp));
  g_return_if_fail (n_args == 0 || args != NULL);
  g_return_if_fail (return_value != NULL);

  context = SWFDEC_AS_OBJECT (function)->context;
  /* just to be sure... */
  SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);
  klass = SWFDEC_AS_FUNCTION_GET_CLASS (function);
  g_assert (klass->call);
  frame = klass->call (function);
  /* FIXME: figure out what to do in these situations */
  if (frame == NULL)
    return;
  if (thisp)
    swfdec_as_frame_set_this (frame, thisp);
  frame->var_object = SWFDEC_AS_OBJECT (frame);
  frame->argc = n_args;
  frame->argv = args;
  frame->return_value = return_value;
  swfdec_as_frame_preload (frame);
  /* FIXME: make this a seperate function? */
  frame->next = context->frame;
  context->frame = frame;
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

