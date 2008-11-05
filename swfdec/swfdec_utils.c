/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_utils.h"

#include <math.h>
#include <string.h>

#include "swfdec_as_internal.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"

gboolean
swfdec_str_case_equal (gconstpointer v1, gconstpointer v2)
{
  const char *s1 = v1;
  const char *s2 = v2;
  
  return g_ascii_strcasecmp (s1, s2) == 0;
}

guint
swfdec_str_case_hash (gconstpointer v)
{
  const signed char *p;
  guint32 h = 0;

  for (p = v; *p != '\0'; p++)
    h = (h << 5) - h + (*p & 0xDF);

  return h;
}

int
swfdec_strcmp (guint version, const char *s1, const char *s2)
{
  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  if (version < 7) {
    return g_ascii_strcasecmp (s1, s2);
  } else {
    return strcmp (s1, s2);
  }
}

int
swfdec_strncmp (guint version, const char *s1, const char *s2, guint n)
{
  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  if (version < 7) {
    return g_ascii_strncasecmp (s1, s2, n);
  } else {
    return strncmp (s1, s2, n);
  }
}

gboolean
swfdec_matrix_from_as_object (cairo_matrix_t *matrix, SwfdecAsObject *object)
{
  SwfdecAsValue *val;
  SwfdecAsContext *cx = object->context;

  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_a);
  if (val == NULL ||
      !isfinite (matrix->xx = swfdec_as_value_to_number (cx, val)))
    return FALSE;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_b);
  if (val == NULL ||
      !isfinite (matrix->yx = swfdec_as_value_to_number (cx, val)))
    return FALSE;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_c);
  if (val == NULL ||
      !isfinite (matrix->xy = swfdec_as_value_to_number (cx, val)))
    return FALSE;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_d);
  if (val == NULL ||
      !isfinite (matrix->yy = swfdec_as_value_to_number (cx, val)))
    return FALSE;

  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_tx);
  if (val == NULL)
      return FALSE;
  matrix->x0 = swfdec_as_value_to_number (cx, val);
  if (!isfinite (matrix->x0))
    matrix->x0 = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_ty);
  if (val == NULL)
      return FALSE;
  matrix->y0 = swfdec_as_value_to_number (cx, val);
  if (!isfinite (matrix->y0))
    matrix->y0 = 0;

  return TRUE;
}

