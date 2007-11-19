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

// static
SWFDEC_AS_NATIVE (14, 0, swfdec_external_interface__initJS)
void
swfdec_external_interface__initJS (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._initJS (static)");
}

SWFDEC_AS_NATIVE (14, 1, swfdec_external_interface__objectID)
void
swfdec_external_interface__objectID (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._objectID (static)");
}

SWFDEC_AS_NATIVE (14, 2, swfdec_external_interface__addCallback)
void
swfdec_external_interface__addCallback (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._addCallback (static)");
}

SWFDEC_AS_NATIVE (14, 3, swfdec_external_interface__evalJS)
void
swfdec_external_interface__evalJS (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._evalJS (static)");
}

SWFDEC_AS_NATIVE (14, 4, swfdec_external_interface__callout)
void
swfdec_external_interface__callout (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._callout (static)");
}

SWFDEC_AS_NATIVE (14, 5, swfdec_external_interface__escapeXML)
void
swfdec_external_interface__escapeXML (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._escapeXML (static)");
}

SWFDEC_AS_NATIVE (14, 6, swfdec_external_interface__unescapeXML)
void
swfdec_external_interface__unescapeXML (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._unescapeXML (static)");
}

SWFDEC_AS_NATIVE (14, 7, swfdec_external_interface__jsQuoteString)
void
swfdec_external_interface__jsQuoteString (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface._jsQuoteString (static)");
}

// properties
SWFDEC_AS_NATIVE (14, 100, swfdec_external_interface_get_available)
void
swfdec_external_interface_get_available (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface.available (static, get)");
}

SWFDEC_AS_NATIVE (14, 101, swfdec_external_interface_set_available)
void
swfdec_external_interface_set_available (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("ExternalInterface.available (static, set)");
}
