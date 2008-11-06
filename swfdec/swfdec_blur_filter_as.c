/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_as_internal.h"
#include "swfdec_blur_filter.h"
#include "swfdec_debug.h"

SWFDEC_AS_NATIVE (1102, 1, swfdec_blur_filter_get_blurX)
void
swfdec_blur_filter_get_blurX (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BLUR_FILTER, &filter, "");

  *ret = swfdec_as_value_from_number (cx, filter->x);
}

SWFDEC_AS_NATIVE (1102, 2, swfdec_blur_filter_set_blurX)
void
swfdec_blur_filter_set_blurX (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;
  double d;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BLUR_FILTER, &filter, "n", &d);

  filter->x = CLAMP (d, 0, 255);
  swfdec_blur_filter_invalidate (filter);
}

SWFDEC_AS_NATIVE (1102, 3, swfdec_blur_filter_get_blurY)
void
swfdec_blur_filter_get_blurY (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BLUR_FILTER, &filter, "");

  *ret = swfdec_as_value_from_number (cx, filter->y);
}

SWFDEC_AS_NATIVE (1102, 4, swfdec_blur_filter_set_blurY)
void
swfdec_blur_filter_set_blurY (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;
  double d;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BLUR_FILTER, &filter, "n", &d);

  filter->y = CLAMP (d, 0, 255);
  swfdec_blur_filter_invalidate (filter);
}

SWFDEC_AS_NATIVE (1102, 5, swfdec_blur_filter_get_quality)
void
swfdec_blur_filter_get_quality (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BLUR_FILTER, &filter, "");

  *ret = swfdec_as_value_from_integer (cx, filter->quality);
}

SWFDEC_AS_NATIVE (1102, 6, swfdec_blur_filter_set_quality)
void
swfdec_blur_filter_set_quality (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;
  int i;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BLUR_FILTER, &filter, "i", &i);

  filter->y = CLAMP (i, 0, 15);
  swfdec_blur_filter_invalidate (filter);
}

// constructor
SWFDEC_AS_NATIVE (1102, 0, swfdec_blur_filter_construct)
void
swfdec_blur_filter_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBlurFilter *filter;
  double x = 4, y = 4;
  int quality = 1;

  SWFDEC_AS_CHECK (0, NULL, "|nni", &x, &y, &quality);

  if (!swfdec_as_context_is_constructing (cx))
    return;

  filter = g_object_new (SWFDEC_TYPE_BLUR_FILTER, "context", cx, NULL);
  filter->x = CLAMP (x, 0, 255);
  filter->y = CLAMP (y, 0, 255);
  filter->quality = CLAMP (quality, 0, 15);

  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (filter));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}
