/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_net_stream.h"
#include "swfdec_player_internal.h"

static JSBool
swfdec_js_video_attach_video (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecNetStream *stream;
  SwfdecVideoMovie *video;

  video = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_VIDEO_MOVIE);
  if (video == NULL)
    return JS_TRUE;

  stream = swfdec_scriptable_from_jsval (cx, argv[0], SWFDEC_TYPE_NET_STREAM);
  if (stream != NULL) {
    swfdec_video_movie_set_input (video, &stream->input);
    return JS_TRUE;
  }
  swfdec_video_movie_set_input (video, NULL);
  return JS_TRUE;
}

static JSBool
swfdec_js_video_clear (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecVideoMovie *video;

  video = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_VIDEO_MOVIE);
  if (video == NULL)
    return JS_TRUE;

  swfdec_video_movie_clear (video);
  return JS_TRUE;
}

static JSFunctionSpec video_methods[] = {
  { "attachVideo",    	swfdec_js_video_attach_video,	1, 0, 0 },
  { "clear",    	swfdec_js_video_clear,		0, 0, 0 },
  {0,0,0,0,0}
};

static void
swfdec_js_video_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecVideo *video;

  video = JS_GetPrivate (cx, obj);
  if (video) {
    SWFDEC_SCRIPTABLE (video)->jsobj = NULL;
    g_object_unref (video);
  }
}

const JSClass video_class = {
    "Video", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   swfdec_js_video_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
swfdec_js_video_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}

void
swfdec_js_add_video (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &video_class, swfdec_js_video_new, 0, NULL, video_methods,
      NULL, NULL);
}

