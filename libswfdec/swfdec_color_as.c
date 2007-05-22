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

#include "swfdec_color_as.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"

G_DEFINE_TYPE (SwfdecMovieColor, swfdec_movie_color, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_movie_color_mark (SwfdecAsObject *object)
{
  SwfdecMovieColor *color = SWFDEC_MOVIE_COLOR (object);

  /* FIXME: unset movie when movie is already dead */
  if (color->movie)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (color->movie));

  SWFDEC_AS_OBJECT_CLASS (swfdec_movie_color_parent_class)->mark (object);
}

static void
swfdec_movie_color_class_init (SwfdecMovieColorClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_movie_color_mark;
}

static void
swfdec_movie_color_init (SwfdecMovieColor *color)
{
}

/*** AS CODE ***/

static void
swfdec_movie_color_getRGB (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  int result;
  SwfdecMovie *movie = SWFDEC_MOVIE_COLOR (obj)->movie;
  
  if (movie == NULL)
    return;

  result = (movie->color_transform.rb << 16) |
	   ((movie->color_transform.gb % 256) << 8) | 
	   (movie->color_transform.bb % 256);
  SWFDEC_AS_VALUE_SET_INT (rval, result);
}

static inline void
add_variable (SwfdecAsObject *obj, const char *name, double value)
{
  SwfdecAsValue val;

  SWFDEC_AS_VALUE_SET_NUMBER (&val, value);
  swfdec_as_object_set_variable (obj, name, &val);
}

static void
swfdec_movie_color_getTransform (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsObject *ret;
  SwfdecMovie *movie = SWFDEC_MOVIE_COLOR (obj)->movie;
  
  if (movie == NULL)
    return;

  ret = swfdec_as_object_new (obj->context);
  if (ret == NULL)
    return;

  add_variable (ret, SWFDEC_AS_STR_ra, movie->color_transform.ra * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_ga, movie->color_transform.ga * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_ba, movie->color_transform.ba * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_aa, movie->color_transform.aa * 100.0 / 256.0);
  add_variable (ret, SWFDEC_AS_STR_rb, movie->color_transform.rb);
  add_variable (ret, SWFDEC_AS_STR_gb, movie->color_transform.gb);
  add_variable (ret, SWFDEC_AS_STR_bb, movie->color_transform.bb);
  add_variable (ret, SWFDEC_AS_STR_ab, movie->color_transform.ab);
  SWFDEC_AS_VALUE_SET_OBJECT (rval, ret);
}

static void
swfdec_movie_color_setRGB (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  guint color;
  SwfdecMovie *movie = SWFDEC_MOVIE_COLOR (obj)->movie;
  
  if (movie == NULL)
    return;

  color = swfdec_as_value_to_integer (obj->context, &argv[0]);

  movie->color_transform.ra = 0;
  movie->color_transform.rb = (color & 0xFF0000) >> 16;
  movie->color_transform.ga = 0;
  movie->color_transform.gb = (color & 0xFF00) >> 8;
  movie->color_transform.ba = 0;
  movie->color_transform.bb = color & 0xFF;
  swfdec_movie_invalidate (movie);
}

static inline void
parse_property (SwfdecAsObject *obj, const char *name, int *target, gboolean scale)
{
  SwfdecAsValue val;
  double d;

  if (!swfdec_as_object_get_variable (obj, name, &val))
    return;
  d = swfdec_as_value_to_number (obj->context, &val);
  if (scale) {
    *target = d * 256.0 / 100.0;
  } else {
    *target = d;
  }
}

static void
swfdec_movie_color_setTransform (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsObject *parse;
  SwfdecMovie *movie = SWFDEC_MOVIE_COLOR (obj)->movie;
  
  if (movie == NULL)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;
  parse = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  parse_property (parse, SWFDEC_AS_STR_ra, &movie->color_transform.ra, TRUE);
  parse_property (parse, SWFDEC_AS_STR_ga, &movie->color_transform.ga, TRUE);
  parse_property (parse, SWFDEC_AS_STR_ba, &movie->color_transform.ba, TRUE);
  parse_property (parse, SWFDEC_AS_STR_aa, &movie->color_transform.aa, TRUE);
  parse_property (parse, SWFDEC_AS_STR_rb, &movie->color_transform.rb, FALSE);
  parse_property (parse, SWFDEC_AS_STR_gb, &movie->color_transform.gb, FALSE);
  parse_property (parse, SWFDEC_AS_STR_bb, &movie->color_transform.bb, FALSE);
  parse_property (parse, SWFDEC_AS_STR_ab, &movie->color_transform.ab, FALSE);
  swfdec_movie_invalidate (movie);
}

static void
swfdec_movie_color_construct (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovieColor *color = SWFDEC_MOVIE_COLOR (obj);

  if (argc > 0 && SWFDEC_AS_VALUE_IS_OBJECT (&argv[0])) {
    SwfdecAsObject *object = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
    if (SWFDEC_IS_MOVIE (object))
      color->movie = SWFDEC_MOVIE (object);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (rval, obj);
}

void
swfdec_movie_color_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *color, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  color = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global, 
      SWFDEC_AS_STR_Color, SWFDEC_TYPE_MOVIE_COLOR, swfdec_movie_color_construct, 0));
  if (!color)
    return;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecMovieColor)))
    return;
  proto = g_object_new (SWFDEC_TYPE_MOVIE_COLOR, NULL);
  swfdec_as_object_add (proto, context, sizeof (SwfdecMovieColor));
  /* set the right properties on the Color object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (color, SWFDEC_AS_STR_prototype, &val);
  /* set the right properties on the Color.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, color);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getRGB, SWFDEC_TYPE_MOVIE_COLOR,
      swfdec_movie_color_getRGB, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getTransform, SWFDEC_TYPE_MOVIE_COLOR,
      swfdec_movie_color_getTransform, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setRGB, SWFDEC_TYPE_MOVIE_COLOR,
      swfdec_movie_color_setRGB, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setTransform, SWFDEC_TYPE_MOVIE_COLOR,
      swfdec_movie_color_setTransform, 1);
}

