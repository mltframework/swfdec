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

G_DEFINE_TYPE (SwfdecAsSuper, swfdec_as_super, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_super_class_init (SwfdecAsSuperClass *klass)
{
}

static void
swfdec_as_super_init (SwfdecAsSuper *super)
{
}

SwfdecAsObject *
swfdec_as_super_new (SwfdecAsContext *context)
{
  SwfdecAsObject *ret;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsSuper)))
    return NULL;
  ret = g_object_new (SWFDEC_TYPE_AS_SUPER, NULL);
  swfdec_as_object_add (ret, context, sizeof (SwfdecAsSuper));
  return ret;
}

