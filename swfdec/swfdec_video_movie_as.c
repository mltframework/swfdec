/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_video.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_net_stream.h"
#include "swfdec_player_internal.h"
#include "swfdec_sandbox.h"

SWFDEC_AS_NATIVE (667, 1, swfdec_video_attach_video)
void
swfdec_video_attach_video (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecVideoMovie *video;
  SwfdecAsObject *o;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_VIDEO_MOVIE, &video, "O", &o);

  if (o == NULL || !SWFDEC_IS_VIDEO_PROVIDER (o->relay)) {
    SWFDEC_WARNING ("calling attachVideo without a NetStream object");
    swfdec_video_movie_set_provider (video, NULL);
    return;
  }

  swfdec_video_movie_set_provider (video, SWFDEC_VIDEO_PROVIDER (o->relay));
}

SWFDEC_AS_NATIVE (667, 2, swfdec_video_clear)
void
swfdec_video_clear (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecVideoMovie *video;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_VIDEO_MOVIE, &video, "");

  swfdec_video_movie_clear (video);
}

static void
swfdec_video_get_width (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecVideoMovie *video;
  guint w;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_VIDEO_MOVIE, &video, "");

  if (video->provider) {
    w = swfdec_video_provider_get_width (video->provider);
  } else {
    w = 0;
  }
  swfdec_as_value_set_integer (cx, rval, w);
}

static void
swfdec_video_get_height (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecVideoMovie *video;
  guint h;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_VIDEO_MOVIE, &video, "");

  if (video->provider) {
    h = swfdec_video_provider_get_height (video->provider);
  } else {
    h = 0;
  }
  swfdec_as_value_set_integer (cx, rval, h);
}

static void
swfdec_video_get_deblocking (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("Video.deblocking (get)");
  swfdec_as_value_set_integer (cx, rval, 0);
}

static void
swfdec_video_set_deblocking (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("Video.deblocking (set)");
}

static void
swfdec_video_get_smoothing (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("Video.smoothing (get)");
  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

static void
swfdec_video_set_smoothing (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("Video.smoothing (set)");
}

void
swfdec_video_movie_init_properties (SwfdecAsContext *cx)
{
  SwfdecAsValue val;
  SwfdecAsObject *video, *proto;

  // FIXME: We should only initialize if the prototype Object has not been
  // initialized by any object's constructor with native properties
  // (TextField, TextFormat, XML, XMLNode at least)

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_Video, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  video = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (video, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_width, 
      swfdec_video_get_width, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_height, 
      swfdec_video_get_height, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_deblocking, 
      swfdec_video_get_deblocking, swfdec_video_set_deblocking);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_smoothing, 
      swfdec_video_get_smoothing, swfdec_video_set_smoothing);
}


