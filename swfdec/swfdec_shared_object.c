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

// normal
SWFDEC_AS_NATIVE (2106, 0, swfdec_shared_object_connect)
void
swfdec_shared_object_connect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.connect");
}

SWFDEC_AS_NATIVE (2106, 1, swfdec_shared_object_send)
void
swfdec_shared_object_send (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.send");
}

SWFDEC_AS_NATIVE (2106, 2, swfdec_shared_object_flush)
void
swfdec_shared_object_flush (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.flush");
}

SWFDEC_AS_NATIVE (2106, 3, swfdec_shared_object_close)
void
swfdec_shared_object_close (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.close");
}

SWFDEC_AS_NATIVE (2106, 4, swfdec_shared_object_getSize)
void
swfdec_shared_object_getSize (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.getSize");
}

SWFDEC_AS_NATIVE (2106, 5, swfdec_shared_object_setFps)
void
swfdec_shared_object_setFps (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.setFps");
}

SWFDEC_AS_NATIVE (2106, 6, swfdec_shared_object_clear)
void
swfdec_shared_object_clear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("SharedObject.clear");
}
