/* Swfdec
 * Copyright (c) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_CACHED_IMAGE_H_
#define _SWFDEC_CACHED_IMAGE_H_

#include <cairo.h>
#include <swfdec/swfdec_cached.h>
#include <swfdec/swfdec_color.h>

G_BEGIN_DECLS

typedef struct _SwfdecCachedImage SwfdecCachedImage;
typedef struct _SwfdecCachedImageClass SwfdecCachedImageClass;

#define SWFDEC_TYPE_CACHED_IMAGE                    (swfdec_cached_image_get_type())
#define SWFDEC_IS_CACHED_IMAGE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_CACHED_IMAGE))
#define SWFDEC_IS_CACHED_IMAGE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_CACHED_IMAGE))
#define SWFDEC_CACHED_IMAGE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_CACHED_IMAGE, SwfdecCachedImage))
#define SWFDEC_CACHED_IMAGE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_CACHED_IMAGE, SwfdecCachedImageClass))
#define SWFDEC_CACHED_IMAGE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_CACHED_IMAGE, SwfdecCachedImageClass))


struct _SwfdecCachedImage {
  SwfdecCached		cached;

  cairo_surface_t *	surface;	/* the surface */
  SwfdecColorTransform	trans;		/* the transform that has been applied to this surface */
};

struct _SwfdecCachedImageClass
{
  SwfdecCachedClass	cached_class;
};

GType			swfdec_cached_image_get_type	(void);

SwfdecCachedImage *	swfdec_cached_image_new		(cairo_surface_t *	surface,
							 gsize			size);


cairo_surface_t *	swfdec_cached_image_get_surface	(SwfdecCachedImage *	image);
void			swfdec_cached_image_get_color_transform
							(SwfdecCachedImage *	image,
							 SwfdecColorTransform *	trans);
void			swfdec_cached_image_set_color_transform
							(SwfdecCachedImage *	image,
							 const SwfdecColorTransform *trans);

G_END_DECLS
#endif
