/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_as_context.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_movie.h"

/*** AS CODE ***/

static SwfdecMovie *
swfdec_movie_color_get_movie (SwfdecAsObject *object)
{
  SwfdecAsValue val;
  SwfdecAsObject *target;

  if (object == NULL)
    return NULL;

  swfdec_as_object_get_variable (object, SWFDEC_AS_STR_target, &val);
  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&val))
    return NULL;

  target = SWFDEC_AS_VALUE_GET_COMPOSITE (&val);
  if (!SWFDEC_IS_MOVIE (target))
    return NULL;

  return SWFDEC_MOVIE (target);
}

SWFDEC_AS_NATIVE (700, 2, swfdec_movie_color_getRGB)
void
swfdec_movie_color_getRGB (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  int result;
  SwfdecMovie *movie;

  movie = swfdec_movie_color_get_movie (obj);
  
  if (movie == NULL)
    return;

  result = (movie->color_transform.rb << 16) |
	   ((movie->color_transform.gb % 256) << 8) | 
	   (movie->color_transform.bb % 256);
  swfdec_as_value_set_integer (cx, ret, result);
}

static void
add_variable (SwfdecAsObject *obj, const char *name, double value)
{
  SwfdecAsValue val;

  swfdec_as_value_set_number (swfdec_gc_object_get_context (obj), &val, value);
  swfdec_as_object_set_variable (obj, name, &val);
}

SWFDEC_AS_NATIVE (700, 3, swfdec_movie_color_getTransform)
void
swfdec_movie_color_getTransform (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsObject *ret;
  SwfdecMovie *movie;

  movie = swfdec_movie_color_get_movie (obj);
  
  if (movie == NULL)
    return;

  ret = swfdec_as_object_new (cx, SWFDEC_AS_STR_Object, NULL);

  add_variable (ret, SWFDEC_AS_STR_ra, movie->color_transform.ra * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_ga, movie->color_transform.ga * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_ba, movie->color_transform.ba * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_aa, movie->color_transform.aa * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_rb, movie->color_transform.rb);
  add_variable (ret, SWFDEC_AS_STR_gb, movie->color_transform.gb);
  add_variable (ret, SWFDEC_AS_STR_bb, movie->color_transform.bb);
  add_variable (ret, SWFDEC_AS_STR_ab, movie->color_transform.ab);
  SWFDEC_AS_VALUE_SET_COMPOSITE (rval, ret);
}

SWFDEC_AS_NATIVE (700, 0, swfdec_movie_color_setRGB)
void
swfdec_movie_color_setRGB (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  guint color;
  SwfdecMovie *movie;

  if (argc < 1)
    return;

  movie = swfdec_movie_color_get_movie (obj);
  
  if (movie == NULL)
    return;

  color = swfdec_as_value_to_integer (cx, &argv[0]);

  movie->color_transform.ra = 0;
  movie->color_transform.rb = (color & 0xFF0000) >> 16;
  movie->color_transform.ga = 0;
  movie->color_transform.gb = (color & 0xFF00) >> 8;
  movie->color_transform.ba = 0;
  movie->color_transform.bb = color & 0xFF;
  swfdec_movie_invalidate_last (movie);
}

static void
parse_property (SwfdecAsObject *obj, const char *name, int *target, gboolean scale)
{
  SwfdecAsValue val;
  double d;

  if (!swfdec_as_object_get_variable (obj, name, &val))
    return;
  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (obj), &val);
  if (scale) {
    *target = d * 256.0 / 100.0;
  } else {
    *target = d;
  }
}

SWFDEC_AS_NATIVE (700, 1, swfdec_movie_color_setTransform)
void
swfdec_movie_color_setTransform (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsObject *parse;
  SwfdecMovie *movie;

  if (argc < 1)
    return;

  movie = swfdec_movie_color_get_movie (obj);
  
  if (movie == NULL)
    return;

  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&argv[0]))
    return;
  parse = SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[0]);
  parse_property (parse, SWFDEC_AS_STR_ra, &movie->color_transform.ra, TRUE);
  parse_property (parse, SWFDEC_AS_STR_ga, &movie->color_transform.ga, TRUE);
  parse_property (parse, SWFDEC_AS_STR_ba, &movie->color_transform.ba, TRUE);
  parse_property (parse, SWFDEC_AS_STR_aa, &movie->color_transform.aa, TRUE);
  parse_property (parse, SWFDEC_AS_STR_rb, &movie->color_transform.rb, FALSE);
  parse_property (parse, SWFDEC_AS_STR_gb, &movie->color_transform.gb, FALSE);
  parse_property (parse, SWFDEC_AS_STR_bb, &movie->color_transform.bb, FALSE);
  parse_property (parse, SWFDEC_AS_STR_ab, &movie->color_transform.ab, FALSE);
  swfdec_movie_invalidate_last (movie);
}
