/* Swfdec
 * Copyright (C) 2007-2008 Pekka Lampila <pekka.lampila@iki.fi>
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
#include "swfdec_as_strings.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecTransformAs, swfdec_transform_as, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_transform_as_class_init (SwfdecTransformAsClass *klass)
{
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
  SWFDEC_STUB ("Transform.matrix (get)");
}

SWFDEC_AS_NATIVE (1106, 102, swfdec_transform_as_set_matrix)
void
swfdec_transform_as_set_matrix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.matrix (set)");
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

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &transform, "");

  if (transform->target == NULL)
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret,
    SWFDEC_AS_OBJECT (swfdec_color_transform_as_new_from_transform (cx,
	&transform->target->color_transform)));
}

SWFDEC_AS_NATIVE (1106, 106, swfdec_transform_as_set_colorTransform)
void
swfdec_transform_as_set_colorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTransformAs *self;
  SwfdecColorTransformAs *transform_as;
  SwfdecColorTransform *transform;
  SwfdecAsObject *color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TRANSFORM_AS, &self, "o", &color);

  if (self->target == NULL)
    return;

  if (!SWFDEC_IS_COLOR_TRANSFORM_AS (color))
    return;

  transform_as = SWFDEC_COLOR_TRANSFORM_AS (color);
  transform = &self->target->color_transform;

  transform->ra = CLAMP (transform_as->ra * 256.0, 0, 256);
  transform->ga = CLAMP (transform_as->ga * 256.0, 0, 256);
  transform->ba = CLAMP (transform_as->ba * 256.0, 0, 256);
  transform->aa = CLAMP (transform_as->aa * 256.0, 0, 256);
  transform->rb = CLAMP (transform_as->rb, -256, 256);
  transform->gb = CLAMP (transform_as->gb, -256, 256);
  transform->bb = CLAMP (transform_as->bb, -256, 256);
  transform->ab = CLAMP (transform_as->ab, -256, 256);
}

SWFDEC_AS_NATIVE (1106, 107, swfdec_transform_as_get_concatenatedColorTransform)
void
swfdec_transform_as_get_concatenatedColorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedColorTransform (get)");
}

SWFDEC_AS_NATIVE (1106, 108, swfdec_transform_as_set_concatenatedColorTransform)
void
swfdec_transform_as_set_concatenatedColorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedColorTransform (set)");
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
SWFDEC_AS_CONSTRUCTOR (1106, 0, swfdec_transform_as_construct, swfdec_transform_as_get_type)
void
swfdec_transform_as_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!cx->frame->construct)
    return;

  if (argc < 1 ||
      !SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]) ||
      !SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]))) {
    SWFDEC_FIXME ("new Transform without movieclip should give undefined");
    return;
  }

  SWFDEC_TRANSFORM_AS (object)->target =
    SWFDEC_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]));
}

SwfdecTransformAs *
swfdec_transform_as_new (SwfdecAsContext *context, SwfdecMovie *target)
{
  SwfdecAsValue val;
  SwfdecTransformAs *transform;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (SWFDEC_IS_MOVIE (target), NULL);

  size = sizeof (SwfdecTransformAs);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  transform = g_object_new (SWFDEC_TYPE_TRANSFORM_AS, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (transform), context, size);

  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_flash, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (&val),
	SWFDEC_AS_STR_geom, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (&val),
	  SWFDEC_AS_STR_Transform, &val);
      if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
	swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (transform),
	    SWFDEC_AS_VALUE_GET_OBJECT (&val));
      }
    }
  }

  transform->target = target;

  return transform;
}
