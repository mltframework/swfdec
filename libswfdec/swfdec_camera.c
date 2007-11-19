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

SWFDEC_AS_NATIVE (2102, 200, swfdec_camera_get)
void
swfdec_camera_get (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.get (static)");
}

SWFDEC_AS_NATIVE (2102, 201, swfdec_camera_names_get)
void
swfdec_camera_names_get (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.names (static)");
}

SWFDEC_AS_NATIVE (2102, 0, swfdec_camera_setMode)
void
swfdec_camera_setMode (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.setMode");
}

SWFDEC_AS_NATIVE (2102, 1, swfdec_camera_setQuality)
void
swfdec_camera_setQuality (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.setQuality");
}

SWFDEC_AS_NATIVE (2102, 2, swfdec_camera_setKeyFrameInterval)
void
swfdec_camera_setKeyFrameInterval (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.setKeyFrameInterval");
}

SWFDEC_AS_NATIVE (2102, 3, swfdec_camera_setMotionLevel)
void
swfdec_camera_setMotionLevel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.setMotionLevel");
}

SWFDEC_AS_NATIVE (2102, 4, swfdec_camera_setLoopback)
void
swfdec_camera_setLoopback (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.setLoopback");
}

SWFDEC_AS_NATIVE (2102, 5, swfdec_camera_setCursor)
void
swfdec_camera_setCursor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Camera.setCursor");
}
