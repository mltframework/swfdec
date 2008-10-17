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

#include "swfdec_movie_clip_loader.h"
#include "swfdec_as_array.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"


G_DEFINE_TYPE (SwfdecMovieClipLoader, swfdec_movie_clip_loader, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_movie_clip_loader_class_init (SwfdecMovieClipLoaderClass *klass)
{
}

static void
swfdec_movie_clip_loader_init (SwfdecMovieClipLoader *movie_clip_loader)
{
}

SWFDEC_AS_CONSTRUCTOR (112, 0, swfdec_movie_clip_loader_construct, swfdec_movie_clip_loader_get_type)
void 
swfdec_movie_clip_loader_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsValue val;
  SwfdecAsObject *array;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  array = swfdec_as_array_new (cx);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  swfdec_as_array_push (SWFDEC_AS_ARRAY (array), &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, array);
  swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR__listeners, 
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
}

SWFDEC_AS_NATIVE (112, 100, swfdec_movie_clip_loader_loadClip)
void 
swfdec_movie_clip_loader_loadClip (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovieClipLoader *loader;
  const char *url;
  SwfdecAsValue target;
  gboolean result;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE_CLIP_LOADER, &loader, "sv", &url, &target);

  result = swfdec_resource_load_movie (SWFDEC_PLAYER (cx), &target, url, 
      NULL, loader);
  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, result);
}

SWFDEC_AS_NATIVE (112, 102, swfdec_movie_clip_loader_unloadClip)
void 
swfdec_movie_clip_loader_unloadClip (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovieClipLoader *loader;
  SwfdecAsValue target;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE_CLIP_LOADER, &loader, "v", &target);

  swfdec_resource_load_movie (SWFDEC_PLAYER (cx), &target, "", NULL, loader);
}

SWFDEC_AS_NATIVE (112, 101, swfdec_movie_clip_loader_getProgress)
void 
swfdec_movie_clip_loader_getProgress (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovieClipLoader *loader;
  SwfdecMovie *movie;
  SwfdecAsObject *ret;
  const char *target;
  SwfdecResource *resource;
  SwfdecAsValue loaded, total;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE_CLIP_LOADER, &loader, "s", &target);

  movie = swfdec_player_get_movie_from_string (SWFDEC_PLAYER (cx), target);
  if (movie == NULL)
    return;
  ret = swfdec_as_object_new_empty (cx);
  SWFDEC_AS_VALUE_SET_OBJECT (rval, ret);
  resource = swfdec_movie_get_own_resource (movie);
  if (resource == NULL || resource->decoder == NULL) {
    swfdec_as_value_set_integer (cx, &loaded, 0);
    swfdec_as_value_set_integer (cx, &total, 0);
  } else {
    swfdec_as_value_set_integer (cx, &loaded, resource->decoder->bytes_loaded);
    swfdec_as_value_set_integer (cx, &total, resource->decoder->bytes_total);
  }
  swfdec_as_object_set_variable (ret, SWFDEC_AS_STR_bytesLoaded, &loaded);
  swfdec_as_object_set_variable (ret, SWFDEC_AS_STR_bytesTotal, &total);
}

