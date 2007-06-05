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

#include "swfdec_as_script_function.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_stack.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsScriptFunction, swfdec_as_script_function, SWFDEC_TYPE_AS_FUNCTION)

static SwfdecAsFrame *
swfdec_as_script_function_call (SwfdecAsFunction *function)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (function);
  SwfdecAsFrame *frame;

  frame = swfdec_as_frame_new (SWFDEC_AS_OBJECT (function)->context, script->script);
  SWFDEC_AS_SCOPE (frame)->next = script->scope;
  frame->function = function;
  return frame;
}

static void
swfdec_as_script_function_dispose (GObject *object)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (object);

  if (script->script) {
    swfdec_script_unref (script->script);
    script->script = NULL;
  }

  G_OBJECT_CLASS (swfdec_as_script_function_parent_class)->dispose (object);
}

static void
swfdec_as_script_function_mark (SwfdecAsObject *object)
{
  SwfdecAsScriptFunction *script= SWFDEC_AS_SCRIPT_FUNCTION (object);

  if (script->scope)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (script->scope));

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_script_function_parent_class)->mark (object);
}

static void
swfdec_as_script_function_class_init (SwfdecAsScriptFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

  object_class->dispose = swfdec_as_script_function_dispose;

  asobject_class->mark = swfdec_as_script_function_mark;

  function_class->call = swfdec_as_script_function_call;
}

static void
swfdec_as_script_function_init (SwfdecAsScriptFunction *script_function)
{
}

SwfdecAsFunction *
swfdec_as_script_function_new (SwfdecAsScope *scope)
{
  SwfdecAsValue val;
  SwfdecAsFunction *fun;
  SwfdecAsObject *proto;

  g_return_val_if_fail (SWFDEC_IS_AS_SCOPE (scope), NULL);

  fun = swfdec_as_function_create (SWFDEC_AS_OBJECT (scope)->context, 
      SWFDEC_TYPE_AS_SCRIPT_FUNCTION, sizeof (SwfdecAsScriptFunction));
  if (fun == NULL)
    return NULL;
  proto = swfdec_as_object_new (SWFDEC_AS_OBJECT (scope)->context);
  if (proto == NULL)
    return NULL;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (fun));
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (fun), SWFDEC_AS_STR_prototype, &val);
  SWFDEC_AS_SCRIPT_FUNCTION (fun)->scope = scope;

  return fun;
}

