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

#include "swfdec_as_scope.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"

/**
 * SwfdecAsScope:
 * @see_also: #SwfdecAsFrame, #SwfdecAsWith
 *
 * A scope is the abstract base class for the #SwfdecAsWith and #SwfdecAsFrame 
 * classes. It is used to resolve variable references inside ActionScript.
 * Consider the following simple ActionScript code:
 * <informalexample><programlisting>
 * foo = bar;
 * </programlisting></informalexample>
 * The variables foo and bar have to be resolved here, so that the script
 * engine knows, what object to set them on. This is done by walking the scope
 * chain. This chain is build up using With and Frame objects.
 */
G_DEFINE_ABSTRACT_TYPE (SwfdecAsScope, swfdec_as_scope, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_scope_dispose (GObject *object)
{
  //SwfdecAsScope *scope = SWFDEC_AS_SCOPE (object);

  G_OBJECT_CLASS (swfdec_as_scope_parent_class)->dispose (object);
}

static void
swfdec_as_scope_mark (SwfdecAsObject *object)
{
  SwfdecAsScope *scope = SWFDEC_AS_SCOPE (object);

  if (scope->next)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (scope->next));

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_scope_parent_class)->mark (object);
}

static void
swfdec_as_scope_class_init (SwfdecAsScopeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_scope_dispose;

  asobject_class->mark = swfdec_as_scope_mark;
}

static void
swfdec_as_scope_init (SwfdecAsScope *scope)
{
}

