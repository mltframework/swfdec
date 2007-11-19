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

SWFDEC_AS_NATIVE (1102, 1, swfdec_blur_filter_get_blurX)
void
swfdec_blur_filter_get_blurX (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter.blurX (get)");
}

SWFDEC_AS_NATIVE (1102, 2, swfdec_blur_filter_set_blurX)
void
swfdec_blur_filter_set_blurX (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter.blurX (set)");
}

SWFDEC_AS_NATIVE (1102, 3, swfdec_blur_filter_get_blurY)
void
swfdec_blur_filter_get_blurY (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter.blurY (get)");
}

SWFDEC_AS_NATIVE (1102, 4, swfdec_blur_filter_set_blurY)
void
swfdec_blur_filter_set_blurY (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter.blurY (set)");
}

SWFDEC_AS_NATIVE (1102, 5, swfdec_blur_filter_get_quality)
void
swfdec_blur_filter_get_quality (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter.quality (get)");
}

SWFDEC_AS_NATIVE (1102, 6, swfdec_blur_filter_set_quality)
void
swfdec_blur_filter_set_quality (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter.quality (set)");
}

// constructor
SWFDEC_AS_NATIVE (1102, 0, swfdec_blur_filter_construct)
void
swfdec_blur_filter_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BlurFilter");
}
