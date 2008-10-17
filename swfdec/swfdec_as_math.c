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

#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_debug.h"

/*** AS CODE ***/

// Note: All Math function call valueOf for two arguments, whether those are
// needed or not

#define MATH_FUN(name) \
void \
swfdec_as_math_ ## name (SwfdecAsContext *cx, SwfdecAsObject *object, \
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret) \
{ \
  double d, unused; \
\
  swfdec_as_value_set_number (cx, ret, NAN); \
  SWFDEC_AS_CHECK (0, NULL, "n|n", &d, &unused); \
\
  d = name (d); \
  swfdec_as_value_set_number (cx, ret, d); \
}

SWFDEC_AS_NATIVE (200, 16, swfdec_as_math_acos)
MATH_FUN (acos)
SWFDEC_AS_NATIVE (200, 15, swfdec_as_math_asin)
MATH_FUN (asin)
SWFDEC_AS_NATIVE (200, 14, swfdec_as_math_atan)
MATH_FUN (atan)
SWFDEC_AS_NATIVE (200, 13, swfdec_as_math_ceil)
MATH_FUN (ceil)
SWFDEC_AS_NATIVE (200, 4, swfdec_as_math_cos)
MATH_FUN (cos)
SWFDEC_AS_NATIVE (200, 7, swfdec_as_math_exp)
MATH_FUN (exp)
SWFDEC_AS_NATIVE (200, 12, swfdec_as_math_floor)
MATH_FUN (floor)
SWFDEC_AS_NATIVE (200, 8, swfdec_as_math_log)
MATH_FUN (log)
SWFDEC_AS_NATIVE (200, 3, swfdec_as_math_sin)
MATH_FUN (sin)
SWFDEC_AS_NATIVE (200, 9, swfdec_as_math_sqrt)
MATH_FUN (sqrt)
SWFDEC_AS_NATIVE (200, 6, swfdec_as_math_tan)
MATH_FUN (tan)

SWFDEC_AS_NATIVE (200, 0, swfdec_as_math_abs)
void
swfdec_as_math_abs (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double d, unused;

  swfdec_as_value_set_number (cx, ret, NAN);
  SWFDEC_AS_CHECK (0, NULL, "n|n", &d, &unused);

  swfdec_as_value_set_number (cx, ret, fabs (d));
}

SWFDEC_AS_NATIVE (200, 5, swfdec_as_math_atan2)
void
swfdec_as_math_atan2 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x, y;

  swfdec_as_value_set_number (cx, ret, NAN);
  SWFDEC_AS_CHECK (0, NULL, "nn", &y, &x);

  swfdec_as_value_set_number (cx, ret, atan2 (y, x));
}

SWFDEC_AS_NATIVE (200, 2, swfdec_as_math_max)
void
swfdec_as_math_max (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x, y;

  if (argc == 0) {
    swfdec_as_value_set_number (cx, ret, -HUGE_VAL);
  } else {
    swfdec_as_value_set_number (cx, ret, NAN);
  }
  SWFDEC_AS_CHECK (0, NULL, "nn", &x, &y);

  swfdec_as_value_set_number (cx, ret, isnan (x) || isnan (y) ? NAN : MAX (x, y));
}

SWFDEC_AS_NATIVE (200, 1, swfdec_as_math_min)
void
swfdec_as_math_min (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x, y;

  if (argc == 0) {
    swfdec_as_value_set_number (cx, ret, HUGE_VAL);
  } else {
    swfdec_as_value_set_number (cx, ret, NAN);
  }
  SWFDEC_AS_CHECK (0, NULL, "nn", &x, &y);

  swfdec_as_value_set_number (cx, ret, isnan (x) || isnan (y) ? NAN : MIN (x, y));
}

SWFDEC_AS_NATIVE (200, 17, swfdec_as_math_pow)
void
swfdec_as_math_pow (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double x, y;

  swfdec_as_value_set_number (cx, ret, NAN);
  SWFDEC_AS_CHECK (0, NULL, "nn", &x, &y);

  swfdec_as_value_set_number (cx, ret, isfinite (x) ? pow (x, y): NAN);
}

SWFDEC_AS_NATIVE (200, 11, swfdec_as_math_random)
void
swfdec_as_math_random (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double unused, unused2;

  swfdec_as_value_set_number (cx, ret, NAN);
  SWFDEC_AS_CHECK (0, NULL, "|nn", &unused, &unused2);

  swfdec_as_value_set_number (cx, ret, g_rand_double (cx->rand));
}

SWFDEC_AS_NATIVE (200, 10, swfdec_as_math_round)
void
swfdec_as_math_round (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double d, unused;

  swfdec_as_value_set_number (cx, ret, NAN);
  SWFDEC_AS_CHECK (0, NULL, "n|n", &d, &unused);

  swfdec_as_value_set_number (cx, ret, floor (d + 0.5));
}
