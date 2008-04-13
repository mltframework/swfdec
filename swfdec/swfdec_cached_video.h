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

#ifndef _SWFDEC_CACHED_VIDEO_H_
#define _SWFDEC_CACHED_VIDEO_H_

#include <cairo.h>
#include <swfdec/swfdec_cached.h>

G_BEGIN_DECLS

typedef struct _SwfdecCachedVideo SwfdecCachedVideo;
typedef struct _SwfdecCachedVideoClass SwfdecCachedVideoClass;

#define SWFDEC_TYPE_CACHED_VIDEO                    (swfdec_cached_video_get_type())
#define SWFDEC_IS_CACHED_VIDEO(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_CACHED_VIDEO))
#define SWFDEC_IS_CACHED_VIDEO_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_CACHED_VIDEO))
#define SWFDEC_CACHED_VIDEO(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_CACHED_VIDEO, SwfdecCachedVideo))
#define SWFDEC_CACHED_VIDEO_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_CACHED_VIDEO, SwfdecCachedVideoClass))
#define SWFDEC_CACHED_VIDEO_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_CACHED_VIDEO, SwfdecCachedVideoClass))


struct _SwfdecCachedVideo {
  SwfdecCached		cached;

  guint			frame;		/* number of the frame */
  guint			width;		/* relevant width of surface */
  guint			height;		/* relevant height of surface */

  cairo_surface_t *	surface;
};

struct _SwfdecCachedVideoClass
{
  SwfdecCachedClass	cached_class;
};

GType			swfdec_cached_video_get_type	(void);

SwfdecCachedVideo *	swfdec_cached_video_new		(cairo_surface_t *	surface,
							 gsize			size);


cairo_surface_t *	swfdec_cached_video_get_surface	(SwfdecCachedVideo *	video);
guint			swfdec_cached_video_get_frame	(SwfdecCachedVideo *	video);
void			swfdec_cached_video_set_frame	(SwfdecCachedVideo *	video,
							 guint			frame);
void			swfdec_cached_video_get_size	(SwfdecCachedVideo *	video,
							 guint *		width,
							 guint *		height);
void			swfdec_cached_video_set_size	(SwfdecCachedVideo *	video,
							 guint			width,
							 guint			height);


G_END_DECLS
#endif
