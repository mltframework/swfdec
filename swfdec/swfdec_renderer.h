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

#ifndef _SWFDEC_RENDERER_H_
#define _SWFDEC_RENDERER_H_

#include <cairo.h>
#include <swfdec/swfdec_player.h>

G_BEGIN_DECLS

//typedef struct _SwfdecRenderer SwfdecRenderer;
typedef struct _SwfdecRendererPrivate SwfdecRendererPrivate;
typedef struct _SwfdecRendererClass SwfdecRendererClass;

#define SWFDEC_TYPE_RENDERER                    (swfdec_renderer_get_type())
#define SWFDEC_IS_RENDERER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_RENDERER))
#define SWFDEC_IS_RENDERER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_RENDERER))
#define SWFDEC_RENDERER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_RENDERER, SwfdecRenderer))
#define SWFDEC_RENDERER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_RENDERER, SwfdecRendererClass))
#define SWFDEC_RENDERER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_RENDERER, SwfdecRendererClass))


struct _SwfdecRenderer {
  GObject		object;

  SwfdecRendererPrivate *priv;
};

struct _SwfdecRendererClass
{
  /*< private >*/
  GObjectClass		object_class;

  /*< public >*/
  cairo_surface_t *	(* create_similar)		(SwfdecRenderer *	renderer,
							 cairo_surface_t *	surface);
  cairo_surface_t *	(* create_for_data)		(SwfdecRenderer *	renderer,
							 guint8 *		data,
							 cairo_format_t		format,
							 guint			width,
							 guint			height,
							 guint			rowstride);
};

GType			swfdec_renderer_get_type	(void);

SwfdecRenderer *	swfdec_renderer_new		(cairo_surface_t *	surface);
SwfdecRenderer *	swfdec_renderer_new_for_player	(cairo_surface_t *	surface,
							 SwfdecPlayer *		player);

cairo_surface_t *	swfdec_renderer_get_surface	(SwfdecRenderer *	renderer);


G_END_DECLS
#endif
