/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include <string.h>

#include "swfdec_gc_object.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"

/**
 * SECTION:SwfdecGcObject
 * @title: SwfdecGcObject
 * @short_description: the base object type for garbage-collected objects
 *
 * This type is the basic garbage-collected object in Swfdec. It contains the
 * simple facilities for required by the garbage collector. The initial 
 * reference of this object will be owned by the context that created it and
 * will be released automatically when no other object references it anymore
 * in the garbage collection cycle.
 *
 * Note that you cannot know the lifetime of a #SwfdecGcObject, since scripts 
 * may assign it as a variable to other objects. So you should not assume to 
 * know when an object gets removed.
 */

/**
 * SwfdecGcObject:
 *
 * If you want to add custom objects to the garbage collection lifecycle, you
 * need to subclass this object as this object is abstract. Note that you have
 * to provide a valid #SwfdecAsContext whenever you construct objects of this
 * type.
 */

/**
 * SwfdecGcObjectClass:
 * @mark: Called in the mark phase of garbage collection. Mark all the 
 *        garbage-collected object you still use here using the marking 
 *        functions such as swfdec_gc_object_mark() or swfdec_as_string_mark()
 *
 * This is the base class for garbage-collected objects.
 */

enum {
  PROP_0,
  PROP_CONTEXT
};

G_DEFINE_ABSTRACT_TYPE (SwfdecGcObject, swfdec_gc_object, G_TYPE_OBJECT)

static gsize
swfdec_gc_object_get_size (SwfdecGcObject *object)
{
  GTypeQuery query;

  /* FIXME: This only uses the size of the public instance but doesn't include
   * private members. http://bugzilla.gnome.org/show_bug.cgi?id=354457 blocks
   * this. */
  g_type_query (G_OBJECT_TYPE (object), &query);
  return query.instance_size;
}

static void
swfdec_gc_object_dispose (GObject *gobject)
{
  SwfdecGcObject *object = SWFDEC_GC_OBJECT (gobject);

  swfdec_as_context_unuse_mem (object->context, swfdec_gc_object_get_size (object));
  G_OBJECT_CLASS (swfdec_gc_object_parent_class)->dispose (gobject);
}

static void
swfdec_gc_object_do_mark (SwfdecGcObject *object)
{
}

static GObject *
swfdec_gc_object_constructor (GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties)
{
  GObject *gobject;
  SwfdecGcObject *object;
  SwfdecAsContext *context;

  gobject = G_OBJECT_CLASS (swfdec_gc_object_parent_class)->constructor (type, 
      n_construct_properties, construct_properties);
  object = SWFDEC_GC_OBJECT (gobject);

  context = object->context;
  swfdec_as_context_use_mem (context, swfdec_gc_object_get_size (object));
  object->next = context->gc_objects;
  context->gc_objects = object;

  return gobject;
}

static void
swfdec_gc_object_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecGcObject *gc = SWFDEC_GC_OBJECT (object);

  switch (param_id) {
    case PROP_CONTEXT:
      g_value_set_object (value, gc->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_gc_object_set_property (GObject *object, guint param_id, const GValue *value, 
    GParamSpec * pspec)
{
  SwfdecGcObject *gc = SWFDEC_GC_OBJECT (object);

  switch (param_id) {
    case PROP_CONTEXT:
      gc->context = g_value_get_object (value);
      g_assert (gc->context != NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_gc_object_class_init (SwfdecGcObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_gc_object_dispose;
  object_class->set_property = swfdec_gc_object_set_property;
  object_class->get_property = swfdec_gc_object_get_property;
  object_class->constructor = swfdec_gc_object_constructor;

  g_object_class_install_property (object_class, PROP_CONTEXT,
      g_param_spec_object ("context", "context", "context managing this object",
	  SWFDEC_TYPE_AS_CONTEXT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  klass->mark = swfdec_gc_object_do_mark;
}

static void
swfdec_gc_object_init (SwfdecGcObject *object)
{
}

/**
 * swfdec_gc_object_get_context:
 * @object: a #SwfdecGcObject. This function takes a gpointer argument only to
 *          save you from having to cast it manually. For language bindings, 
 *          please treat this argument as having the #SwfdecGcObject type.
 *
 * Gets the context that garbage-collects this object.
 *
 * Returns: the context this object belongs to
 **/
SwfdecAsContext *
swfdec_gc_object_get_context (gpointer object)
{
  g_return_val_if_fail (SWFDEC_IS_GC_OBJECT (object), NULL);

  return SWFDEC_GC_OBJECT (object)->context;
}

/**
 * swfdec_gc_object_mark:
 * @object: a #SwfdecGcObject. This function takes a gpointer argument only to
 *          save you from having to cast it manually. For language bindings, 
 *          please treat this argument as having the #SwfdecGcObject type.
 *
 * Mark @object as being in use. Calling this function is only valid during
 * the marking phase of garbage collection.
 **/
void
swfdec_gc_object_mark (gpointer object)
{
  SwfdecGcObject *gc = object;
  SwfdecGcObjectClass *klass;

  g_return_if_fail (SWFDEC_IS_GC_OBJECT (object));

  if (gc->flags & SWFDEC_AS_GC_MARK)
    return;
  gc->flags |= SWFDEC_AS_GC_MARK;
  klass = SWFDEC_GC_OBJECT_GET_CLASS (gc);
  g_assert (klass->mark);
  klass->mark (gc);
}

