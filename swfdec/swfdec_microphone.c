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

SWFDEC_AS_NATIVE (2104, 200, swfdec_microphone_get)
void
swfdec_microphone_get (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Microphone.get (static)");
}

SWFDEC_AS_NATIVE (2104, 201, swfdec_microphone_names_get)
void
swfdec_microphone_names_get (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Microphone.names (static, get)");
}

SWFDEC_AS_NATIVE (2104, 0, swfdec_microphone_setSilenceLevel)
void
swfdec_microphone_setSilenceLevel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Microphone.setSilenceLevel");
}

SWFDEC_AS_NATIVE (2104, 1, swfdec_microphone_setRate)
void
swfdec_microphone_setRate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Microphone.setRate");
}

SWFDEC_AS_NATIVE (2104, 2, swfdec_microphone_setGain)
void
swfdec_microphone_setGain (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Microphone.setGain");
}

SWFDEC_AS_NATIVE (2104, 3, swfdec_microphone_setUseEchoSuppression)
void
swfdec_microphone_setUseEchoSuppression (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Microphone.setUseEchoSuppression");
}
