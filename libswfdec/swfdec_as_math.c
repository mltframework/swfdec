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

#define MATH_FUN(name) \
static void \
swfdec_as_math_ ## name (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret) \
{ \
  double d = swfdec_as_value_to_number (object->context, &argv[0]); \
\
  d = name (d); \
  SWFDEC_AS_VALUE_SET_NUMBER (ret, d); \
}

MATH_FUN (acos)
MATH_FUN (asin)
MATH_FUN (atan)
MATH_FUN (ceil)
MATH_FUN (cos)
MATH_FUN (exp)
MATH_FUN (floor)
MATH_FUN (log)
MATH_FUN (round)
MATH_FUN (sin)
MATH_FUN (sqrt)
MATH_FUN (tan)

static void
swfdec_as_math_abs (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double v = swfdec_as_value_to_number (object->context, &argv[0]);
  SWFDEC_AS_VALUE_SET_NUMBER (ret, fabs (v));
}

static void
swfdec_as_math_atan2 (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double y = swfdec_as_value_to_number (object->context, &argv[0]);
  double x = swfdec_as_value_to_number (object->context, &argv[1]);

  SWFDEC_AS_VALUE_SET_NUMBER (ret, atan2 (y, x));
}

static void
swfdec_as_math_max (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x = swfdec_as_value_to_number (object->context, &argv[0]);
  double y = swfdec_as_value_to_number (object->context, &argv[1]);

  SWFDEC_AS_VALUE_SET_NUMBER (ret, MAX (x, y));
}

static void
swfdec_as_math_min (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x = swfdec_as_value_to_number (object->context, &argv[0]);
  double y = swfdec_as_value_to_number (object->context, &argv[1]);

  SWFDEC_AS_VALUE_SET_NUMBER (ret, MIN (x, y));
}

static void
swfdec_as_math_pow (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x = swfdec_as_value_to_number (object->context, &argv[0]);
  double y = swfdec_as_value_to_number (object->context, &argv[1]);

  SWFDEC_AS_VALUE_SET_NUMBER (ret, pow (x, y));
}

static void
swfdec_as_math_random (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_NUMBER (ret, g_rand_double (object->context->rand));
}

/* define some math constants if glib doesn't have them */
#ifndef G_LOG10E
#define G_LOG10E 0.43429448190325182765
#endif
#ifndef G_LOG2E
#define G_LOG2E 1.4426950408889634074
#endif
#ifndef G_SQRT1_2
#define G_SQRT1_2 0.70710678118654752440
#endif
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
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_E);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_E, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_LN10);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_LN10, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_LN2);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_LN2, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_LOG10E);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_LOG10E, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_LOG2E);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_LOG2E, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_PI);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_PI, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_SQRT1_2);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_SQRT1_2, &val);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, G_SQRT2);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_SQRT2, &val);

  /* set the right functions on the Math object */
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_abs, 0, swfdec_as_math_abs, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_acos, 0, swfdec_as_math_acos, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_asin, 0, swfdec_as_math_asin, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_atan, 0, swfdec_as_math_atan, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_atan2, 0, swfdec_as_math_atan2, 2);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_ceil, 0, swfdec_as_math_ceil, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_cos, 0, swfdec_as_math_cos, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_exp, 0, swfdec_as_math_exp, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_floor, 0, swfdec_as_math_floor, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_log, 0, swfdec_as_math_log, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_min, 0, swfdec_as_math_min, 2);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_max, 0, swfdec_as_math_max, 2);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_pow, 0, swfdec_as_math_pow, 2);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_random, 0, swfdec_as_math_random, 0);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_round, 0, swfdec_as_math_round, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_sin, 0, swfdec_as_math_sin, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_sqrt, 0, swfdec_as_math_sqrt, 1);
  swfdec_as_object_add_function (math, SWFDEC_AS_STR_tan, 0, swfdec_as_math_tan, 1);
}

