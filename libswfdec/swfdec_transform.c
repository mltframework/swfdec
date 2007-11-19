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

#include "swfdec_as_internal.h"
#include "swfdec_debug.h"

// properties
SWFDEC_AS_NATIVE (1106, 101, swfdec_transform_get_matrix)
void
swfdec_transform_get_matrix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.matrix (get)");
}

SWFDEC_AS_NATIVE (1106, 102, swfdec_transform_set_matrix)
void
swfdec_transform_set_matrix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.matrix (set)");
}

SWFDEC_AS_NATIVE (1106, 103, swfdec_transform_get_concatenatedMatrix)
void
swfdec_transform_get_concatenatedMatrix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedMatrix (get)");
}

SWFDEC_AS_NATIVE (1106, 104, swfdec_transform_set_concatenatedMatrix)
void
swfdec_transform_set_concatenatedMatrix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedMatrix (set)");
}

SWFDEC_AS_NATIVE (1106, 105, swfdec_transform_get_colorTransform)
void
swfdec_transform_get_colorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.colorTransform (get)");
}

SWFDEC_AS_NATIVE (1106, 106, swfdec_transform_set_colorTransform)
void
swfdec_transform_set_colorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.colorTransform (set)");
}

SWFDEC_AS_NATIVE (1106, 107, swfdec_transform_get_concatenatedColorTransform)
void
swfdec_transform_get_concatenatedColorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedColorTransform (get)");
}

SWFDEC_AS_NATIVE (1106, 108, swfdec_transform_set_concatenatedColorTransform)
void
swfdec_transform_set_concatenatedColorTransform (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.concatenatedColorTransform (set)");
}

SWFDEC_AS_NATIVE (1106, 109, swfdec_transform_get_pixelBounds)
void
swfdec_transform_get_pixelBounds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.pixelBounds (get)");
}

SWFDEC_AS_NATIVE (1106, 110, swfdec_transform_set_pixelBounds)
void
swfdec_transform_set_pixelBounds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform.pixelBounds (set)");
}

// constructor
SWFDEC_AS_NATIVE (1106, 0, swfdec_transform_construct)
void
swfdec_transform_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Transform");
}
