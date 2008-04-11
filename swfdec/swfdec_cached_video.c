/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_cached_video.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecCachedVideo, swfdec_cached_video, SWFDEC_TYPE_CACHED)

static void
swfdec_cached_video_dispose (GObject *object)
{
  SwfdecCachedVideo *video = SWFDEC_CACHED_VIDEO (object);

  if (video->surface) {
    cairo_surface_destroy (video->surface);
    video->surface = NULL;
  }

  G_OBJECT_CLASS (swfdec_cached_video_parent_class)->dispose (object);
}

static void
swfdec_cached_video_class_init (SwfdecCachedVideoClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_cached_video_dispose;
}

static void
swfdec_cached_video_init (SwfdecCachedVideo *cached)
{
}

SwfdecCachedVideo *
swfdec_cached_video_new (cairo_surface_t *surface, gsize size)
{
  SwfdecCachedVideo *video;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (size > 0, NULL);

  size += sizeof (SwfdecCachedVideo);
  video = g_object_new (SWFDEC_TYPE_CACHED_VIDEO, "size", size, NULL);
  video->surface = cairo_surface_reference (surface);

  return video;
}

cairo_surface_t *
swfdec_cached_video_get_surface (SwfdecCachedVideo *video)
{
  g_return_val_if_fail (SWFDEC_IS_CACHED_VIDEO (video), NULL);

  return cairo_surface_reference (video->surface);
}

guint
swfdec_cached_video_get_frame (SwfdecCachedVideo *video)
{
  g_return_val_if_fail (SWFDEC_IS_CACHED_VIDEO (video), 0);

  return video->frame;
}

void
swfdec_cached_video_set_frame (SwfdecCachedVideo *video, guint frame)
{
  g_return_if_fail (SWFDEC_IS_CACHED_VIDEO (video));

  video->frame = frame;
}

void
swfdec_cached_video_get_size (SwfdecCachedVideo *video, guint *width, guint *height)
{
  g_return_if_fail (SWFDEC_IS_CACHED_VIDEO (video));

  if (width)
    *width = video->width;
  if (height)
    *height = video->height;
}

void
swfdec_cached_video_set_size (SwfdecCachedVideo *video, guint width, guint height)
{
  g_return_if_fail (SWFDEC_IS_CACHED_VIDEO (video));

  video->width = width;
  video->height = height;
}

