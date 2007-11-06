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
#include "swfdec_as_internal.h"
#include "swfdec_debug.h"
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
}

SWFDEC_AS_NATIVE (112, 100, swfdec_movie_clip_loader_loadClip)
void 
swfdec_movie_clip_loader_loadClip (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovieClipLoader *loader;
  const char *url, *target;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE_CLIP_LOADER, &loader, "ss", &url, &target);

  swfdec_resource_load (SWFDEC_PLAYER (cx), target, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL, loader);
}

