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
#include "swfdec_as_frame.h"
#include "swfdec_as_function.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsSuper, swfdec_as_super, SWFDEC_TYPE_AS_FUNCTION)

static SwfdecAsFrame *
swfdec_as_super_call (SwfdecAsFunction *function)
{
  SwfdecAsSuper *super = SWFDEC_AS_SUPER (function);
  SwfdecAsFunctionClass *klass;
  SwfdecAsFrame *frame;

  if (super->constructor == NULL) {
    SWFDEC_FIXME ("figure out what happens when super doesn't have a constructor");
    return NULL;
  }
  klass = SWFDEC_AS_FUNCTION_GET_CLASS (super->constructor);
  frame = klass->call (super->constructor);
  /* FIXME: this is ugly */
  swfdec_as_frame_set_this (frame, super->object);
  return frame;
}

static void
swfdec_as_super_class_init (SwfdecAsSuperClass *klass)
{
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

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
  
  context = SWFDEC_AS_OBJECT (frame)->context;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsSuper)))
    return NULL;
  ret = g_object_new (SWFDEC_TYPE_AS_SUPER, NULL);
  super = SWFDEC_AS_SUPER (ret);
  if (frame->thisp) {
    SwfdecAsValue val;
    super->object = frame->thisp;
    swfdec_as_object_get_variable (frame->thisp, SWFDEC_AS_STR_constructor, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      SwfdecAsObject *constructor = SWFDEC_AS_VALUE_GET_OBJECT (&val);
      swfdec_as_object_get_variable (constructor, SWFDEC_AS_STR___constructor__, &val);
      if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
	super->constructor = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (&val);
	if (!SWFDEC_IS_AS_FUNCTION (super->constructor))
	  super->constructor = NULL;
      }
    }
  }
  swfdec_as_object_add (ret, context, sizeof (SwfdecAsSuper));
  return ret;
}

