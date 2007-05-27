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

#include "swfdec_as_math.h"
#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"

/*** AS CODE ***/

static void
swfdec_as_math_ceil (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double d = swfdec_as_value_to_number (object->context, &argv[0]);

  d = ceil (d);
  SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
}

void
swfdec_as_math_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *math;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  math = swfdec_as_object_new (context);
  if (math == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, math);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_Math, &val);
  /* set the right properties on the Math object */
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_ceil, 0, swfdec_as_math_ceil, 1);
}

