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

#include "swfdec_as_number.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsNumber, swfdec_as_number, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_as_number_class_init (SwfdecAsNumberClass *klass)
{
}

static void
swfdec_as_number_init (SwfdecAsNumber *number)
{
}

/*** AS CODE ***/

SWFDEC_AS_NATIVE (106, 2, swfdec_as_number_construct)
void
swfdec_as_number_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double d;

  if (argc > 0) {
    d = swfdec_as_value_to_number (cx, &argv[0]);
  } else {
    d = NAN;
  }

  if (swfdec_as_context_is_constructing (cx)) {
    SwfdecAsNumber *num = g_object_new (SWFDEC_TYPE_AS_NUMBER, "context", cx, NULL);
    num->number = d;
    swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (num));
    SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
  } else {
    swfdec_as_value_set_number (cx, ret, d);
  }
}

// code adapted from Tamarin's convertDoubleToStringRadix in MathUtils.cpp
// 2008-02-16
static const char *
swfdec_as_number_toStringRadix (SwfdecAsContext *context, double value,
    int radix)
{
  gboolean negative;
  GString *str;
  double left = floor (value);

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_NaN);
  g_return_val_if_fail (radix >= 2 && radix <= 36, SWFDEC_AS_STR_NaN);
  g_return_val_if_fail (!isinf (value) && !isnan (value), SWFDEC_AS_STR_NaN);

  if (value < 0) {
    negative = TRUE;
    value = -value;
  } else {
    negative = FALSE;
  }

  if (value < 1)
    return SWFDEC_AS_STR_0;

  str = g_string_new ("");

  left = floor (value);

  while (left != 0)
  {
    double val = left;
    left = floor (left / radix);
    val -= (left * radix);

    g_string_prepend_c (str,
	(val < 10 ? ((int)val + '0') : ((int)val + ('a' - 10))));
  }

  if (negative)
    g_string_prepend_c (str, '-');

  return swfdec_as_context_give_string (context, g_string_free (str, FALSE));
}

SWFDEC_AS_NATIVE (106, 1, swfdec_as_number_toString)
void
swfdec_as_number_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsNumber *num;
  SwfdecAsValue val;
  const char *s;
  int radix = 10;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_NUMBER, &num, "|i", &radix);

  if (radix == 10 || radix < 2 || radix > 36 || isinf (num->number) ||
      isnan (num->number)) {
    swfdec_as_value_set_number (cx, &val, num->number);
    s = swfdec_as_value_to_string (cx, &val);
  } else {
    s = swfdec_as_number_toStringRadix (cx, num->number, radix);
  }
  SWFDEC_AS_VALUE_SET_STRING (ret, s);
}

SWFDEC_AS_NATIVE (106, 0, swfdec_as_number_valueOf)
void
swfdec_as_number_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsNumber *num;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_NUMBER, &num, "");

  swfdec_as_value_set_number (cx, ret, num->number);
}

// only available as ASnative
SWFDEC_AS_NATIVE (3, 1, swfdec_as_number_old_constructor)
void
swfdec_as_number_old_constructor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("old 'Number' function (only available as ASnative)");
}

// only available as ASnative
SWFDEC_AS_NATIVE (3, 4, swfdec_as_number_old_toString)
void
swfdec_as_number_old_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("old 'Number.prototype.toString' function (only available as ASnative)");
}
