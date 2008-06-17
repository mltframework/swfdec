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

G_DEFINE_TYPE (SwfdecColorTransformAs, swfdec_color_transform_as, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_color_transform_as_class_init (SwfdecColorTransformAsClass *klass)
{
}

static void
swfdec_color_transform_as_init (SwfdecColorTransformAs *transform)
{
  swfdec_color_transform_init_identity (&transform->transform);
}

// properties
SWFDEC_AS_NATIVE (1105, 101, swfdec_color_transform_as_get_alphaMultiplier)
void
swfdec_color_transform_as_get_alphaMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.alphaMultiplier (get)");
}

SWFDEC_AS_NATIVE (1105, 102, swfdec_color_transform_as_set_alphaMultiplier)
void
swfdec_color_transform_as_set_alphaMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.alphaMultiplier (set)");
}

SWFDEC_AS_NATIVE (1105, 103, swfdec_color_transform_as_get_redMultiplier)
void
swfdec_color_transform_as_get_redMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_COLOR_TRANSFORM_AS, &transform, "");

  SWFDEC_AS_VALUE_SET_NUMBER (ret, transform->transform.ra / 256.0);
}

SWFDEC_AS_NATIVE (1105, 104, swfdec_color_transform_as_set_redMultiplier)
void
swfdec_color_transform_as_set_redMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.redMultiplier (set)");
}

SWFDEC_AS_NATIVE (1105, 105, swfdec_color_transform_as_get_greenMultiplier)
void
swfdec_color_transform_as_get_greenMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.greenMultiplier (get)");
}

SWFDEC_AS_NATIVE (1105, 106, swfdec_color_transform_as_set_greenMultiplier)
void
swfdec_color_transform_as_set_greenMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.greenMultiplier (set)");
}

SWFDEC_AS_NATIVE (1105, 107, swfdec_color_transform_as_get_blueMultiplier)
void
swfdec_color_transform_as_get_blueMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.blueMultiplier (get)");
}

SWFDEC_AS_NATIVE (1105, 108, swfdec_color_transform_as_set_blueMultiplier)
void
swfdec_color_transform_as_set_blueMultiplier (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.blueMultiplier (set)");
}

SWFDEC_AS_NATIVE (1105, 109, swfdec_color_transform_as_get_alphaOffset)
void
swfdec_color_transform_as_get_alphaOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.alphaOffset (get)");
}

SWFDEC_AS_NATIVE (1105, 110, swfdec_color_transform_as_set_alphaOffset)
void
swfdec_color_transform_as_set_alphaOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.alphaOffset (set)");
}

SWFDEC_AS_NATIVE (1105, 111, swfdec_color_transform_as_get_redOffset)
void
swfdec_color_transform_as_get_redOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.redOffset (get)");
}

SWFDEC_AS_NATIVE (1105, 112, swfdec_color_transform_as_set_redOffset)
void
swfdec_color_transform_as_set_redOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.redOffset (set)");
}

SWFDEC_AS_NATIVE (1105, 113, swfdec_color_transform_as_get_greenOffset)
void
swfdec_color_transform_as_get_greenOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.greenOffset (get)");
}

SWFDEC_AS_NATIVE (1105, 114, swfdec_color_transform_as_set_greenOffset)
void
swfdec_color_transform_as_set_greenOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.greenOffset (set)");
}

SWFDEC_AS_NATIVE (1105, 115, swfdec_color_transform_as_get_blueOffset)
void
swfdec_color_transform_as_get_blueOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.blueOffset (get)");
}

SWFDEC_AS_NATIVE (1105, 116, swfdec_color_transform_as_set_blueOffset)
void
swfdec_color_transform_as_set_blueOffset (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.blueOffset (set)");
}

SWFDEC_AS_NATIVE (1105, 117, swfdec_color_transform_as_get_rgb)
void
swfdec_color_transform_as_get_rgb (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.rgb (get)");
}

SWFDEC_AS_NATIVE (1105, 118, swfdec_color_transform_as_set_rgb)
void
swfdec_color_transform_as_set_rgb (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.rgb (set)");
}

// normal
SWFDEC_AS_NATIVE (1105, 1, swfdec_color_transform_as_concat)
void
swfdec_color_transform_as_concat (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ColorTransform.concat");
}

// constructor
SWFDEC_AS_CONSTRUCTOR (1105, 0, swfdec_color_transform_as_construct, swfdec_color_transform_as_get_type)
void
swfdec_color_transform_as_construct (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecColorTransformAs *transform;
  guint i;

  if (!cx->frame->construct)
    return;

  if (argc < 8)
    return;

  transform = SWFDEC_COLOR_TRANSFORM_AS (object);

  i = 0;
  transform->transform.ra = 256 * swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.ga = 256 * swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.ba = 256 * swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.aa = 256 * swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.rb = swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.gb = swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.bb = swfdec_as_value_to_number (cx, &argv[i++]);
  transform->transform.ab = swfdec_as_value_to_number (cx, &argv[i++]);
}

SwfdecColorTransformAs *
swfdec_color_transform_as_new_from_transform (SwfdecAsContext *context,
    const SwfdecColorTransform *transform)
{
  SwfdecAsValue val;
  SwfdecColorTransformAs *object;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  size = sizeof (SwfdecColorTransformAs);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  object = g_object_new (SWFDEC_TYPE_COLOR_TRANSFORM_AS, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (object), context, size);

  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_flash, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (&val),
	SWFDEC_AS_STR_geom, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (&val),
	  SWFDEC_AS_STR_ColorTransform, &val);
      if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
	swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (object),
	    SWFDEC_AS_VALUE_GET_OBJECT (&val));
      }
    }
  }

  object->transform = *transform;

  return object;
}
