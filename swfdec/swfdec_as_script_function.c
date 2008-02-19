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
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsScriptFunction, swfdec_as_script_function, SWFDEC_TYPE_AS_FUNCTION)

static SwfdecAsFrame *
swfdec_as_script_function_call (SwfdecAsFunction *function)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (function);
  SwfdecAsFrame *frame;

  frame = swfdec_as_frame_new (SWFDEC_AS_OBJECT (function)->context, script->script);
  if (frame == NULL)
    return NULL;
  frame->scope_chain = g_slist_concat (frame->scope_chain, g_slist_copy (script->scope_chain));
  frame->function = function;
  frame->target = script->target;
  frame->original_target = script->target;
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
  g_slist_free (script->scope_chain);
  script->scope_chain = NULL;

  G_OBJECT_CLASS (swfdec_as_script_function_parent_class)->dispose (object);
}

static void
swfdec_as_script_function_mark (SwfdecAsObject *object)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (object);

  g_slist_foreach (script->scope_chain, (GFunc) swfdec_as_object_mark, NULL);

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_script_function_parent_class)->mark (object);
}

static char *
swfdec_as_script_function_debug (SwfdecAsObject *object)
{
  SwfdecAsScriptFunction *script = SWFDEC_AS_SCRIPT_FUNCTION (object);
  SwfdecScript *s = script->script;
  GString *string;
  guint i;

  string = g_string_new (s->name);
  g_string_append (string, " (");
  for (i = 0; i < s->n_arguments; i++) {
    if (i > 0)
      g_string_append (string, ", ");
    g_string_append (string, s->arguments[i].name);
  }
  g_string_append (string, ")");

  return g_string_free (string, FALSE);
}

static void
swfdec_as_script_function_class_init (SwfdecAsScriptFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

  object_class->dispose = swfdec_as_script_function_dispose;

  asobject_class->mark = swfdec_as_script_function_mark;
  asobject_class->debug = swfdec_as_script_function_debug;

  function_class->call = swfdec_as_script_function_call;
}

static void
swfdec_as_script_function_init (SwfdecAsScriptFunction *script_function)
{
}

SwfdecAsFunction *
swfdec_as_script_function_new (SwfdecAsObject *target, const GSList *scope_chain, SwfdecScript *script)
{
  SwfdecAsValue val;
  SwfdecAsScriptFunction *fun;
  SwfdecAsObject *proto;
  SwfdecAsContext *context;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), NULL);
  g_return_val_if_fail (script != NULL, NULL);

  context = target->context;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsScriptFunction)))
    return NULL;
  fun = g_object_new (SWFDEC_TYPE_AS_SCRIPT_FUNCTION, NULL);
  if (fun == NULL)
    return NULL;
  fun->scope_chain = g_slist_copy ((GSList *) scope_chain);
  fun->script = script;
  fun->target = target;
  swfdec_as_object_add (SWFDEC_AS_OBJECT (fun), context, sizeof (SwfdecAsScriptFunction));
  /* set prototype */
  proto = swfdec_as_object_new_empty (context);
  if (proto == NULL)
    return NULL;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (fun),
      SWFDEC_AS_STR_prototype, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  swfdec_as_function_set_constructor (SWFDEC_AS_FUNCTION (fun));
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (fun));
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR___proto__,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

  return SWFDEC_AS_FUNCTION (fun);
}

