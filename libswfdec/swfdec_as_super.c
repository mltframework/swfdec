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

#include <math.h>

#include "swfdec_as_super.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"

G_DEFINE_TYPE (SwfdecAsSuper, swfdec_as_super, SWFDEC_TYPE_AS_FUNCTION)

static SwfdecAsFrame *
swfdec_as_super_call (SwfdecAsFunction *function)
{
  SwfdecAsSuper *super = SWFDEC_AS_SUPER (function);
  SwfdecAsValue val;
  SwfdecAsFunction *fun;
  SwfdecAsFunctionClass *klass;
  SwfdecAsFrame *frame;

  if (super->object == NULL) {
    SWFDEC_WARNING ("super () called without an object.");
    return NULL;
  }

  swfdec_as_object_get_variable (super->object, SWFDEC_AS_STR___constructor__, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val) ||
      !SWFDEC_IS_AS_FUNCTION (fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (&val)))
    return NULL;

  klass = SWFDEC_AS_FUNCTION_GET_CLASS (fun);
  frame = klass->call (fun);
  if (frame == NULL)
    return NULL;
  /* We set the real function here. 1) swfdec_as_context_run() requires it. 
   * And b) it makes more sense reading the constructor's name than reading "super" 
   * in a debugger
   */
  frame->function = fun;
  frame->construct = frame->next->construct;
  /* FIXME: this is ugly */
  swfdec_as_frame_set_this (frame, super->object);
  return frame;
}

static gboolean
swfdec_as_super_get (SwfdecAsObject *object, SwfdecAsObject *orig,
    const char *variable, SwfdecAsValue *val, guint *flags)
{
  SwfdecAsSuper *super = SWFDEC_AS_SUPER (object);

  if (super->object == NULL) {
    SWFDEC_WARNING ("super.%s () called without an object.", variable);
    return FALSE;
  }
  if (super->object->prototype == NULL) {
    SWFDEC_WARNING ("super.%s () called without a prototype.", variable);
    return FALSE;
  }
  if (!swfdec_as_object_get_variable_and_flags (super->object->prototype, variable, val, NULL, NULL))
    return FALSE;
  *flags = 0;
  return TRUE;
}

static void
swfdec_as_super_set (SwfdecAsObject *object, const char *variable, const SwfdecAsValue *val, guint flags)
{
  /* This seems to be ignored completely */
}

static void
swfdec_as_super_set_flags (SwfdecAsObject *object, const char *variable, guint flags, guint mask)
{
  /* if we have no variables, we also can't set its flags... */
}

static gboolean
swfdec_as_super_delete (SwfdecAsObject *object, const char *variable)
{
  /* if we have no variables... */
  return FALSE;
}

static SwfdecAsObject *
swfdec_as_super_resolve (SwfdecAsObject *object)
{
  SwfdecAsSuper *super = SWFDEC_AS_SUPER (object);

  return super->thisp;
}

static void
swfdec_as_super_class_init (SwfdecAsSuperClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

  asobject_class->get = swfdec_as_super_get;
  asobject_class->set = swfdec_as_super_set;
  asobject_class->set_flags = swfdec_as_super_set_flags;
  asobject_class->del = swfdec_as_super_delete;
  asobject_class->resolve = swfdec_as_super_resolve;

  function_class->call = swfdec_as_super_call;
}

static void
swfdec_as_super_init (SwfdecAsSuper *super)
{
}

SwfdecAsObject *
swfdec_as_super_new (SwfdecAsFrame *frame)
{
  SwfdecAsContext *context;
  SwfdecAsSuper *super;
  SwfdecAsObject *ret;

  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), NULL);
  
  if (frame->thisp == NULL) {
    SWFDEC_FIXME ("found a case where this was NULL, test how super behaves here!");
    return NULL;
  }
  /* functions called on native objects don't get a super object?! */
  if (SWFDEC_IS_MOVIE (frame->thisp))
    return NULL;

  context = SWFDEC_AS_OBJECT (frame)->context;
  if (context->version <= 5 && !frame->construct)
    return NULL;

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsSuper)))
    return NULL;
  ret = g_object_new (SWFDEC_TYPE_AS_SUPER, NULL);
  swfdec_as_object_add (ret, context, sizeof (SwfdecAsSuper));
  super = SWFDEC_AS_SUPER (ret);
  super->thisp = frame->thisp;
  if (context->version <= 5) {
    super->object = NULL;
  } else {
    super->object = frame->thisp->prototype;
  }
  return ret;
}

/**
 * swfdec_as_super_replace:
 * @super: the super object to replace from
 * @function_name: garbage-collected name of the function that was called on 
 *                 @super or %NULL
 *
 * This is an internal function used to replace the current frame's super object
 * with the next one starting from @super. (FIXME: nice explanation!) So when 
 * super.foo () has been called, a new frame is constructed and after that this 
 * function is called with @super and "foo" as @function_name.
 **/
void
swfdec_as_super_replace (SwfdecAsSuper *super, const char *function_name)
{
  SwfdecAsSuper *replace;
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_AS_SUPER (super));

  context = SWFDEC_AS_OBJECT (super)->context;
  replace = SWFDEC_AS_SUPER (context->frame->super);
  if (replace == NULL)
    return;
  if (super->object == NULL || super->object->prototype == NULL) {
    replace->object = NULL;
    return;
  }
  replace->object = super->object->prototype;
  if (function_name && context->version > 6) {
    SwfdecAsObject *res;
    if (swfdec_as_object_get_variable_and_flags (replace->object, 
	  function_name, NULL, NULL, &res) &&
	replace->object != res) {
      while (replace->object->prototype != res)
	replace->object = replace->object->prototype;
    }
  }
}
