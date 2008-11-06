/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
 *		 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_color_matrix_filter.h"

#include <string.h>

#include "swfdec_as_array.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_debug.h"

SWFDEC_AS_NATIVE (1110, 1, swfdec_color_matrix_filter_get_matrix)
void
swfdec_color_matrix_filter_get_matrix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorMatrixFilter *cm;
  SwfdecAsObject *array;
  SwfdecAsValue val[20];
  guint i;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_MATRIX_FILTER, &cm, "");

  for (i = 0; i < 20; i++) {
    val[i] = swfdec_as_value_from_number (cx, cm->matrix[i]);
  }
  array = swfdec_as_array_new (cx);
  swfdec_as_array_append (array, 20, val);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, array);
}

static void
swfdec_color_matrix_filter_do_set_matrix (SwfdecColorMatrixFilter *cm, 
    SwfdecAsObject *array)
{
  SwfdecAsContext *cx = swfdec_gc_object_get_context (cm);
  SwfdecAsValue val;
  int i;

  if (!array->array) {
    memset (cm->matrix, 0, sizeof (double) * 20);
    return;
  }

  for (i = 0; i < 20; i++) {
    if (!swfdec_as_object_get_variable (array, swfdec_as_integer_to_string (cx, i), &val)) {
      cm->matrix[i] = 0;
    } else {
      cm->matrix[i] = swfdec_as_value_to_number (cx, val);
    }
  }
}

SWFDEC_AS_NATIVE (1110, 2, swfdec_color_matrix_filter_set_matrix)
void
swfdec_color_matrix_filter_set_matrix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorMatrixFilter *cm;
  SwfdecAsObject *array;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_MATRIX_FILTER, &cm, "o", &array);

  swfdec_color_matrix_filter_do_set_matrix (cm, array);
}

SWFDEC_AS_NATIVE (1110, 0, swfdec_color_matrix_filter_contruct)
void
swfdec_color_matrix_filter_contruct (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorMatrixFilter *cm;
  SwfdecAsObject *array;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (0, NULL, "o", &array);

  cm = g_object_new (SWFDEC_TYPE_COLOR_MATRIX_FILTER, "context", cx, NULL);
  swfdec_color_matrix_filter_do_set_matrix (cm, array);

  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (cm));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}
