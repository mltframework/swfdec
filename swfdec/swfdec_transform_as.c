/* Swfdec
 * Copyright (C) 2007-2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_transform_as.h"

#include "swfdec_color_transform_as.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_utils.h"

G_DEFINE_TYPE (SwfdecTransformAs, swfdec_transform_as, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_transform_as_mark (SwfdecGcObject *object)
{
  SwfdecTransformAs *trans = SWFDEC_TRANSFORM_AS (object);

  if (trans->target != NULL)
    swfdec_gc_object_mark (trans->target);

  SWFDEC_GC_OBJECT_CLASS (swfdec_transform_as_parent_class)->mark (object);
}

static void
swfdec_transform_as_class_init (SwfdecTransformAsClass *klass)
{
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  gc_class->mark = swfdec_transform_as_mark;
}

static void
swfdec_transform_as_init (SwfdecTransformAs *transform)
{
}

// properties
SWFDEC_AS_NATIVE (1106, 101, swfdec_transform_as_get_matrix)
void
swfdec_transform_as_get_matrix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTransformAs *transform;
  SwfdecAsObject *o;
  cairo_matrix_t *matrix;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &transform, "");
  if (transform->target == NULL)
    return;

  swfdec_movie_update (transform->target);
  matrix = &transform->target->matrix;
  o = swfdec_as_object_new_empty (cx);
  swfdec_as_object_set_constructor_by_name (o, SWFDEC_AS_STR_flash,
	SWFDEC_AS_STR_geom, SWFDEC_AS_STR_Matrix, NULL);

  swfdec_as_value_set_number (cx, &val, matrix->xx);
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_a, &val);
  swfdec_as_value_set_number (cx, &val, matrix->yx);
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_b, &val);
  swfdec_as_value_set_number (cx, &val, matrix->xy);
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_c, &val);
  swfdec_as_value_set_number (cx, &val, matrix->yy);
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_d, &val);
  swfdec_as_value_set_number (cx, &val, matrix->yy);
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_d, &val);
  swfdec_as_value_set_number (cx, &val, SWFDEC_TWIPS_TO_DOUBLE (matrix->x0));
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_tx, &val);
  swfdec_as_value_set_number (cx, &val, SWFDEC_TWIPS_TO_DOUBLE (matrix->y0));
  swfdec_as_object_set_variable (o, SWFDEC_AS_STR_ty, &val);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, o);
}

SWFDEC_AS_NATIVE (1106, 102, swfdec_transform_as_set_matrix)
void
swfdec_transform_as_set_matrix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  cairo_matrix_t tmp;
  SwfdecTransformAs *transform;
  SwfdecAsObject *o;
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &transform, "o", &o);
  if (transform->target == NULL ||
      !swfdec_matrix_from_as_object (&tmp, o))
    return;

  tmp.x0 = SWFDEC_DOUBLE_TO_TWIPS (tmp.x0);
  tmp.y0 = SWFDEC_DOUBLE_TO_TWIPS (tmp.y0);

  /* NB: We don't use begin/end_update_matrix() here, because Flash is
   * broken enough to not want that. */
  movie = transform->target;
  swfdec_movie_invalidate_next (movie);

  movie->matrix = tmp;

  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
  swfdec_matrix_ensure_invertible (&movie->matrix, &movie->inverse_matrix);
  g_signal_emit_by_name (movie, "matrix-changed");
}

SWFDEC_AS_NATIVE (1106, 103, swfdec_transform_as_get_concatenatedMatrix)
void
swfdec_transform_as_get_concatenatedMatrix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedMatrix (get)");
}

SWFDEC_AS_NATIVE (1106, 104, swfdec_transform_as_set_concatenatedMatrix)
void
swfdec_transform_as_set_concatenatedMatrix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedMatrix (set)");
}

SWFDEC_AS_NATIVE (1106, 105, swfdec_transform_as_get_colorTransform)
void
swfdec_transform_as_get_colorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTransformAs *transform;
  SwfdecColorTransformAs *ctrans;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &transform, "");

  if (transform->target == NULL)
    return;

  ctrans = swfdec_color_transform_as_new_from_transform (cx,
      &transform->target->color_transform);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (ctrans)));
}

SWFDEC_AS_NATIVE (1106, 106, swfdec_transform_as_set_colorTransform)
void
swfdec_transform_as_set_colorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTransformAs *self;
  SwfdecColorTransformAs *transform_as;
  SwfdecAsObject *color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &self, "o", &color);

  if (self->target == NULL)
    return;

  if (color == NULL || !SWFDEC_IS_COLOR_TRANSFORM_AS (color->relay))
    return;

  transform_as = SWFDEC_COLOR_TRANSFORM_AS (color->relay);

  swfdec_color_transform_get_transform (transform_as, &self->target->color_transform);
}

SWFDEC_AS_NATIVE (1106, 107, swfdec_transform_as_get_concatenatedColorTransform)
void
swfdec_transform_as_get_concatenatedColorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTransformAs *self;
  SwfdecColorTransform chain;
  SwfdecColorTransformAs *ctrans;
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &self, "");

  if (self->target == NULL)
    return;

  chain = self->target->color_transform;

  for (movie = self->target->parent; movie != NULL; movie = movie->parent) {
    swfdec_color_transform_chain (&chain, &movie->color_transform, &chain);
  }

  ctrans = swfdec_color_transform_as_new_from_transform (cx, &chain);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (ctrans)));
}

SWFDEC_AS_NATIVE (1106, 108, swfdec_transform_as_set_concatenatedColorTransform)
void
swfdec_transform_as_set_concatenatedColorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  // read-only
}

SWFDEC_AS_NATIVE (1106, 109, swfdec_transform_as_get_pixelBounds)
void
swfdec_transform_as_get_pixelBounds (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.pixelBounds (get)");
}

SWFDEC_AS_NATIVE (1106, 110, swfdec_transform_as_set_pixelBounds)
void
swfdec_transform_as_set_pixelBounds (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.pixelBounds (set)");
}

// constructor
SWFDEC_AS_NATIVE (1106, 0, swfdec_transform_as_construct)
void
swfdec_transform_as_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTransformAs *trans;
  SwfdecMovie *movie;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (0, NULL, "M", &movie);

  trans = g_object_new (SWFDEC_TYPE_TRANSFORM_AS, "context", cx, NULL);
  trans->target = SWFDEC_AS_VALUE_GET_MOVIE (&argv[0]);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (trans));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

SwfdecTransformAs *
swfdec_transform_as_new (SwfdecAsContext *context, SwfdecMovie *target)
{
  SwfdecTransformAs *transform;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (SWFDEC_IS_MOVIE (target), NULL);

  transform = g_object_new (SWFDEC_TYPE_TRANSFORM_AS, "context", context, NULL);
  transform->target = target;

  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_constructor_by_name (object,
      SWFDEC_AS_STR_flash, SWFDEC_AS_STR_geom, SWFDEC_AS_STR_Transform, NULL);

  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (transform));

  return transform;
}
