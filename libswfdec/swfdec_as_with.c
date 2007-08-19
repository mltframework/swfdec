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

#include "swfdec_as_with.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsWith, swfdec_as_with, SWFDEC_TYPE_AS_SCOPE)

static void
swfdec_as_with_mark (SwfdecAsObject *object)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);

  swfdec_as_object_mark (with->object);

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_with_parent_class)->mark (object);
}

static SwfdecAsObject *
swfdec_as_with_resolve (SwfdecAsObject *object)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);

  return with->object;
}

static gboolean
swfdec_as_with_get (SwfdecAsObject *object, SwfdecAsObject *orig,
    const char *variable, SwfdecAsValue *val, guint *flags)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);

  if (orig != object) {
    SWFDEC_FIXME ("write tests for this case");
  }
  return swfdec_as_object_get_variable_and_flags (with->object, variable, val, flags, NULL);
}

static void
swfdec_as_with_set (SwfdecAsObject *object, const char *variable,
    const SwfdecAsValue *val, guint flags)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);
  SwfdecAsObjectClass *klass = SWFDEC_AS_OBJECT_GET_CLASS (with->object);

  klass->set (with->object, variable, val, flags);
}

static void
swfdec_as_with_set_flags (SwfdecAsObject *object, const char *variable,
    guint flags, guint mask)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);
  SwfdecAsObjectClass *klass = SWFDEC_AS_OBJECT_GET_CLASS (with->object);

  klass->set_flags (with->object, variable, flags, mask);
}

static gboolean
swfdec_as_with_delete (SwfdecAsObject *object, const char *variable)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);

  return swfdec_as_object_delete_variable (with->object, variable);
}

static gboolean
swfdec_as_with_foreach (SwfdecAsObject *object, SwfdecAsVariableForeach func,
    gpointer data)
{
  SwfdecAsWith *with = SWFDEC_AS_WITH (object);
  SwfdecAsObjectClass *klass = SWFDEC_AS_OBJECT_GET_CLASS (with->object);

  return klass->foreach (with->object, func, data);
}

static void
swfdec_as_with_class_init (SwfdecAsWithClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_as_with_mark;
  asobject_class->get = swfdec_as_with_get;
  asobject_class->set = swfdec_as_with_set;
  asobject_class->set_flags = swfdec_as_with_set_flags;
  asobject_class->del = swfdec_as_with_delete;
  asobject_class->foreach = swfdec_as_with_foreach;
  asobject_class->resolve = swfdec_as_with_resolve;
}

static void
swfdec_as_with_init (SwfdecAsWith *with)
{
}

SwfdecAsScope *
swfdec_as_with_new (SwfdecAsObject *object, const guint8 *startpc, guint n_bytes)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;
  SwfdecAsScope *scope;
  SwfdecAsWith *with;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);

  context = object->context;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsWith)))
    return NULL;
  with = g_object_new (SWFDEC_TYPE_AS_WITH, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (with), context, sizeof (SwfdecAsWith));
  scope = SWFDEC_AS_SCOPE (with);
  frame = context->frame;
  with->object = object;
  scope->startpc = startpc;
  scope->endpc = startpc + n_bytes;
  scope->next = frame->scope;
  frame->scope = scope;

  return scope;
}

