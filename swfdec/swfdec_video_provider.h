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

#ifndef __SWFDEC_VIDEO_PROVIDER_H__
#define __SWFDEC_VIDEO_PROVIDER_H__

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


#define SWFDEC_TYPE_VIDEO_PROVIDER                (swfdec_video_provider_get_type ())
#define SWFDEC_VIDEO_PROVIDER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO_PROVIDER, SwfdecVideoProvider))
#define SWFDEC_IS_VIDEO_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO_PROVIDER))
#define SWFDEC_VIDEO_PROVIDER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), SWFDEC_TYPE_VIDEO_PROVIDER, SwfdecVideoProviderInterface))

typedef struct _SwfdecVideoProvider SwfdecVideoProvider; /* dummy object */
typedef struct _SwfdecVideoProviderInterface SwfdecVideoProviderInterface;

struct _SwfdecVideoProviderInterface {
  GTypeInterface	interface;

  /* called when movie ratio changed */
  void			(* set_ratio)				(SwfdecVideoProvider *	provider,
								 guint			ratio);
  /* called to request the current image */
  cairo_surface_t *	(* get_image)				(SwfdecVideoProvider *	provider,
								 SwfdecRenderer *	renderer,
								 guint *		width,
								 guint *		height);
};

GType			swfdec_video_provider_get_type		(void) G_GNUC_CONST;

cairo_surface_t *     	swfdec_video_provider_get_image		(SwfdecVideoProvider *	provider,
								 SwfdecRenderer *	renderer,
								 guint *		width,
								 guint *		height);
void			swfdec_video_provider_set_ratio		(SwfdecVideoProvider *	provider,
								 guint			ratio);

/* for subclasses */
void			swfdec_video_provider_new_image		(SwfdecVideoProvider *	provider);


G_END_DECLS

#endif /* __SWFDEC_VIDEO_PROVIDER_H__ */
