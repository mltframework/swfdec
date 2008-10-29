/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_color_transform_as.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecColorTransformAs, swfdec_color_transform_as, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_color_transform_as_class_init (SwfdecColorTransformAsClass *klass)
{
}

static void
swfdec_color_transform_as_init (SwfdecColorTransformAs *transform)
{
  transform->ra = 1;
  transform->ga = 1;
  transform->ba = 1;
  transform->aa = 1;
  transform->rb = 0;
  transform->gb = 0;
  transform->bb = 0;
  transform->ab = 0;
}

// properties
SWFDEC_AS_NATIVE (1105, 101, swfdec_color_transform_as_get_alphaMultiplier)
void
swfdec_color_transform_as_get_alphaMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->aa);
}

SWFDEC_AS_NATIVE (1105, 102, swfdec_color_transform_as_set_alphaMultiplier)
void
swfdec_color_transform_as_set_alphaMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->aa = value;
}

SWFDEC_AS_NATIVE (1105, 103, swfdec_color_transform_as_get_redMultiplier)
void
swfdec_color_transform_as_get_redMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->ra);
}

SWFDEC_AS_NATIVE (1105, 104, swfdec_color_transform_as_set_redMultiplier)
void
swfdec_color_transform_as_set_redMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->ra = value;
}

SWFDEC_AS_NATIVE (1105, 105, swfdec_color_transform_as_get_greenMultiplier)
void
swfdec_color_transform_as_get_greenMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->ga);
}

SWFDEC_AS_NATIVE (1105, 106, swfdec_color_transform_as_set_greenMultiplier)
void
swfdec_color_transform_as_set_greenMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->ga = value;
}

SWFDEC_AS_NATIVE (1105, 107, swfdec_color_transform_as_get_blueMultiplier)
void
swfdec_color_transform_as_get_blueMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->ba);
}

SWFDEC_AS_NATIVE (1105, 108, swfdec_color_transform_as_set_blueMultiplier)
void
swfdec_color_transform_as_set_blueMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->ba = value;
}

SWFDEC_AS_NATIVE (1105, 109, swfdec_color_transform_as_get_alphaOffset)
void
swfdec_color_transform_as_get_alphaOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->ab);
}

SWFDEC_AS_NATIVE (1105, 110, swfdec_color_transform_as_set_alphaOffset)
void
swfdec_color_transform_as_set_alphaOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->ab = value;
}

SWFDEC_AS_NATIVE (1105, 111, swfdec_color_transform_as_get_redOffset)
void
swfdec_color_transform_as_get_redOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->rb);
}

SWFDEC_AS_NATIVE (1105, 112, swfdec_color_transform_as_set_redOffset)
void
swfdec_color_transform_as_set_redOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->rb = value;
}

SWFDEC_AS_NATIVE (1105, 113, swfdec_color_transform_as_get_greenOffset)
void
swfdec_color_transform_as_get_greenOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->gb);
}

SWFDEC_AS_NATIVE (1105, 114, swfdec_color_transform_as_set_greenOffset)
void
swfdec_color_transform_as_set_greenOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->gb = value;
}

SWFDEC_AS_NATIVE (1105, 115, swfdec_color_transform_as_get_blueOffset)
void
swfdec_color_transform_as_get_blueOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  swfdec_as_value_set_number (cx, ret, transform->bb);
}

SWFDEC_AS_NATIVE (1105, 116, swfdec_color_transform_as_set_blueOffset)
void
swfdec_color_transform_as_set_blueOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  double value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "n", &value);

  transform->bb = value;
}

SWFDEC_AS_NATIVE (1105, 117, swfdec_color_transform_as_get_rgb)
void
swfdec_color_transform_as_get_rgb (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  int value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  // important to calculate the value this way, to get right results with
  // negative values
  value = swfdec_as_double_to_integer (transform->rb) << 16;
  value |= swfdec_as_double_to_integer (transform->gb) << 8;
  value |= swfdec_as_double_to_integer (transform->bb);

  swfdec_as_value_set_integer (cx, ret, value);
}

SWFDEC_AS_NATIVE (1105, 118, swfdec_color_transform_as_set_rgb)
void
swfdec_color_transform_as_set_rgb (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  int value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "i", &value);

  transform->ra = 0;
  transform->ga = 0;
  transform->ba = 0;
  transform->rb = (value & 0xFF0000) >> 16;
  transform->gb = (value & 0x00FF00) >> 8;
  transform->bb = (value & 0x0000FF);
}

// normal
SWFDEC_AS_NATIVE (1105, 1, swfdec_color_transform_as_concat)
void
swfdec_color_transform_as_concat (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform, *other;
  SwfdecAsObject *other_object;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "o",
      &other_object);

  if (!SWFDEC_IS_COLOR_TRANSFORM_AS (other_object->relay))
    return;
  other = SWFDEC_COLOR_TRANSFORM_AS (other_object->relay);

  transform->rb += (transform->ra * other->rb);
  transform->gb += (transform->ga * other->gb);
  transform->bb += (transform->ba * other->bb);
  transform->ab += (transform->aa * other->ab);
  transform->ra *= other->ra;
  transform->ga *= other->ga;
  transform->ba *= other->ba;
  transform->aa *= other->aa;
}

// constructor
SWFDEC_AS_NATIVE (1105, 0, swfdec_color_transform_as_construct)
void
swfdec_color_transform_as_construct (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  transform = g_object_new (SWFDEC_TYPE_COLOR_TRANSFORM_AS, "context", cx, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (transform));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);

  if (argc < 8)
    return;

  SWFDEC_AS_CHECK (0, NULL, "nnnnnnnn", 
      &transform->ra, &transform->ga, &transform->ba, &transform->aa,
      &transform->rb, &transform->gb, &transform->bb, &transform->ab);
}

SwfdecColorTransformAs *
swfdec_color_transform_as_new_from_transform (SwfdecAsContext *context,
    const SwfdecColorTransform *transform)
{
  SwfdecColorTransformAs *transform_as;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  transform_as = g_object_new (SWFDEC_TYPE_COLOR_TRANSFORM_AS, "context", context, NULL);
  /* do it this way so the constructor isn't called */
  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (transform_as));
  swfdec_as_object_set_constructor_by_name (object, 
      SWFDEC_AS_STR_flash, SWFDEC_AS_STR_geom, SWFDEC_AS_STR_ColorTransform, NULL);

  transform_as->ra = transform->ra / 256.0;
  transform_as->ga = transform->ga / 256.0;
  transform_as->ba = transform->ba / 256.0;
  transform_as->aa = transform->aa / 256.0;
  transform_as->rb = transform->rb;
  transform_as->gb = transform->gb;
  transform_as->bb = transform->bb;
  transform_as->ab = transform->ab;

  return transform_as;
}

void
swfdec_color_transform_get_transform (SwfdecColorTransformAs *trans,
    SwfdecColorTransform *ctrans)
{
  g_return_if_fail (SWFDEC_IS_COLOR_TRANSFORM_AS (trans));
  g_return_if_fail (ctrans != NULL);

  SWFDEC_FIXME ("This conversion needs serious testing with NaN and overflows");
  ctrans->mask = FALSE;
  ctrans->ra = trans->ra * 256.0;
  ctrans->ga = trans->ga * 256.0;
  ctrans->ba = trans->ba * 256.0;
  ctrans->aa = trans->aa * 256.0;
  ctrans->rb = trans->rb;
  ctrans->gb = trans->gb;
  ctrans->bb = trans->bb;
  ctrans->ab = trans->ab;
}

