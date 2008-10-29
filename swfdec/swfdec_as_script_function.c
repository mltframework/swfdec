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

#include "swfdec_as_script_function.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsScriptFunction, swfdec_as_script_function, SWFDEC_TYPE_AS_FUNCTION)

static void
swfdec_as_script_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, 
    gboolean construct, SwfdecAsObject *super_reference, guint n_args, 
    const SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (function);
  SwfdecAsContext *context;
  SwfdecSandbox *old_sandbox = NULL;
  SwfdecAsFrame frame = { NULL, };

  /* just to be sure... */
  if (return_value)
    SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);

  context = swfdec_gc_object_get_context (function);
  /* do security checks */
  if (script->sandbox != NULL &&
      script->sandbox != (old_sandbox = swfdec_sandbox_get (SWFDEC_PLAYER (context)))) {
    if (!swfdec_sandbox_allow (script->sandbox, old_sandbox))
      return;
    swfdec_sandbox_unuse (old_sandbox);
    swfdec_sandbox_use (script->sandbox);
  }

  swfdec_as_frame_init (&frame, swfdec_gc_object_get_context (function), script->script);
  frame.scope_chain = g_slist_concat (frame.scope_chain, g_slist_copy (script->scope_chain));
  frame.function = function;
  frame.target = script->target;
  frame.original_target = script->target;
  /* FIXME: figure out what to do in these situations?
   * It's a problem when called inside swfdec_as_function_call () as the
   * user of that function expects success, but super may fail here */
  /* second check especially for super object */
  if (thisp != NULL && frame.thisp == NULL) {
    swfdec_as_frame_set_this (&frame, swfdec_as_object_resolve (thisp));
  }
  frame.argc = n_args;
  frame.argv = args;
  frame.return_value = return_value;
  frame.construct = construct;
  if (super_reference == NULL) {
    /* don't create a super object */
  } else if (thisp != NULL) {
    swfdec_as_super_new (&frame, thisp, super_reference);
  } else {
    // FIXME: Does the super object really reference the function when thisp is NULL?
    swfdec_as_super_new (&frame, 
	swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (function)), super_reference);
  }
  swfdec_as_frame_preload (&frame);
  swfdec_as_context_run (context);

  if (old_sandbox) {
    swfdec_sandbox_unuse (script->sandbox);
    swfdec_sandbox_use (old_sandbox);
  }
}

static void
swfdec_as_script_function_dispose (GObject *object)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (object);

  if (script->script) {
    swfdec_script_unref (script->script);
    script->script = NULL;
  }
  g_slist_free (script->scope_chain);
  script->scope_chain = NULL;

  G_OBJECT_CLASS (swfdec_as_script_function_parent_class)->dispose (object);
}

static void
swfdec_as_script_function_mark (SwfdecGcObject *object)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (object);

  g_slist_foreach (script->scope_chain, (GFunc) swfdec_gc_object_mark, NULL);
  if (script->sandbox)
    swfdec_gc_object_mark (script->sandbox);

  SWFDEC_GC_OBJECT_CLASS (swfdec_as_script_function_parent_class)->mark (object);
}

static void
swfdec_as_script_function_class_init (SwfdecAsScriptFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

  object_class->dispose = swfdec_as_script_function_dispose;

  gc_class->mark = swfdec_as_script_function_mark;

  function_class->call = swfdec_as_script_function_call;
}

static void
swfdec_as_script_function_init (SwfdecAsScriptFunction *script_function)
{
}

SwfdecAsFunction *
swfdec_as_script_function_new (SwfdecAsObject *target, const GSList *scope_chain, SwfdecScript *script)
{
  SwfdecAsValue val, *tmp;
  SwfdecAsScriptFunction *fun;
  SwfdecAsObject *proto, *object;
  SwfdecAsContext *context;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), NULL);
  g_return_val_if_fail (script != NULL, NULL);

  context = swfdec_gc_object_get_context (target);
  fun = g_object_new (SWFDEC_TYPE_AS_SCRIPT_FUNCTION, "context", context, NULL);
  fun->scope_chain = g_slist_copy ((GSList *) scope_chain);
  fun->script = script;
  fun->target = target;

  /* if context is a flash player, copy current sandbox for security checking.
   * FIXME: export this somehow? */
  if (SWFDEC_IS_PLAYER (context))
    fun->sandbox = swfdec_sandbox_get (SWFDEC_PLAYER (context));

  object = swfdec_as_object_new_empty (context);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (fun));
  swfdec_as_object_set_constructor_by_name (object, SWFDEC_AS_STR_Function, NULL);
  swfdec_as_object_set_variable_flags (object, SWFDEC_AS_STR___proto__, 
      SWFDEC_AS_VARIABLE_VERSION_6_UP);

  /* set prototype */
  proto = swfdec_as_object_new_empty (context);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, proto);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_prototype, 
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, object);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  tmp = swfdec_as_object_peek_variable (context->global, SWFDEC_AS_STR_Object);
  if (tmp && SWFDEC_AS_VALUE_IS_COMPOSITE (tmp)) {
    tmp = swfdec_as_object_peek_variable (SWFDEC_AS_VALUE_GET_COMPOSITE (tmp),
	SWFDEC_AS_STR_prototype);
    if (tmp) {
      swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR___proto__,
	  tmp, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
    }
  }

  return SWFDEC_AS_FUNCTION (fun);
}

