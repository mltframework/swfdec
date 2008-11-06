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
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"

G_DEFINE_TYPE (SwfdecAsSuper, swfdec_as_super, SWFDEC_TYPE_AS_FUNCTION)

static void
swfdec_as_super_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, 
    gboolean construct, SwfdecAsObject *super_reference, guint n_args, 
    const SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsSuper *super = SWFDEC_AS_SUPER (function);
  SwfdecAsFunction *fun;
  SwfdecAsValue val;

  if (super->object == NULL) {
    SWFDEC_WARNING ("super () called without an object.");
    return;
  }

  swfdec_as_object_get_variable (super->object, SWFDEC_AS_STR___constructor__, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (val) ||
      !SWFDEC_IS_AS_FUNCTION (fun = (SwfdecAsFunction *) (SWFDEC_AS_VALUE_GET_OBJECT (val)->relay)))
    return;

  if (construct) {
    SWFDEC_FIXME ("What happens with \"new super()\"?");
  }
  swfdec_as_function_call_full (fun, super->thisp, construct || 
      swfdec_as_context_is_constructing (swfdec_gc_object_get_context (super)),
      super->object->prototype, n_args, args, return_value);
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

void
swfdec_as_super_new (SwfdecAsFrame *frame, SwfdecAsObject *thisp, SwfdecAsObject *ref)
{
  SwfdecAsContext *context;
  SwfdecAsObject *object;
  SwfdecAsSuper *super;

  g_return_if_fail (frame != NULL);
  g_return_if_fail (thisp != NULL);
  
  if (frame->super != NULL)
    return;
  context = thisp->context;
  if (context->version <= 5)
    return;

  super = g_object_new (SWFDEC_TYPE_AS_SUPER, "context", context, NULL);
  frame->super = super;
  super->thisp = swfdec_as_object_resolve (thisp);
  if (context->version <= 5) {
    super->object = NULL;
  } else {
    super->object = ref;
  }

  object = swfdec_as_object_new_empty (context);
  object->super = TRUE;
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (super));
}

SwfdecAsObject *
swfdec_as_super_resolve_property (SwfdecAsSuper *super, const char *name)
{
  SwfdecAsObject *ref;
  SwfdecAsContext *context;
	  
  g_return_val_if_fail (SWFDEC_IS_AS_SUPER (super), NULL);

  if (super->object == NULL)
    return NULL;
  ref = super->object->prototype;
  if (ref == NULL)
    return NULL;
  context = swfdec_gc_object_get_context (super);
  if (name && context->version > 6) {
    /* skip prototypes to find the next one that has this function defined */
    SwfdecAsObject *res;
    if (swfdec_as_object_get_variable_and_flags (ref, 
         name, NULL, NULL, &res) && ref != res) {
      while (ref->prototype != res) {
        ref = ref->prototype;
        g_assert (ref);
      }
    }
  }
  return ref;
}

