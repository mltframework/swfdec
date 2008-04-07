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

#include "swfdec_cached_image.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecCachedImage, swfdec_cached_image, SWFDEC_TYPE_CACHED)

static void
swfdec_cached_image_dispose (GObject *object)
{
  SwfdecCachedImage *image = SWFDEC_CACHED_IMAGE (object);

  if (image->surface) {
    cairo_surface_destroy (image->surface);
    image->surface = NULL;
  }

  G_OBJECT_CLASS (swfdec_cached_image_parent_class)->dispose (object);
}

static void
swfdec_cached_image_class_init (SwfdecCachedImageClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_cached_image_dispose;
}

static void
swfdec_cached_image_init (SwfdecCachedImage *cached)
{
  swfdec_color_transform_init_identity (&cached->trans);
}

SwfdecCachedImage *
swfdec_cached_image_new (guint8 *image_data, cairo_format_t format,
    guint width, guint height, guint rowstride)
{
  SwfdecCachedImage *image;
  static const cairo_user_data_key_t key;
  gulong size;

  g_return_val_if_fail (image_data != NULL, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (rowstride > 0, NULL);

  /* FIXME: sizeof (cairo_surface_t) */
  size = sizeof (SwfdecCachedImage) + height * rowstride;
  image = g_object_new (SWFDEC_TYPE_CACHED_IMAGE, "size", size, NULL);
  image->surface = cairo_image_surface_create_for_data (image_data, format,
      width, height, rowstride);
  if (cairo_surface_set_user_data (image->surface, &key, image_data, g_free) != CAIRO_STATUS_SUCCESS) {
    /* guess we should abort here due to OOM? */
    g_warning ("failed to register data free function");
  }

  return image;
}

SwfdecCachedImage *
swfdec_cached_image_new_for_surface (cairo_surface_t *surface, gsize size)
{
  SwfdecCachedImage *image;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (size > 0, NULL);

  size += sizeof (SwfdecCachedImage);
  image = g_object_new (SWFDEC_TYPE_CACHED_IMAGE, "size", size, NULL);
  image->surface = cairo_surface_reference (surface);

  return image;
}

cairo_surface_t *
swfdec_cached_image_get_surface (SwfdecCachedImage *image)
{
  g_return_val_if_fail (SWFDEC_IS_CACHED_IMAGE (image), NULL);

  return cairo_surface_reference (image->surface);
}

