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

#ifndef _SWFDEC_DRAW_H_
#define _SWFDEC_DRAW_H_

#include <glib-object.h>
#include <cairo.h>
#include <swfdec/swfdec_swf_decoder.h>
#include <swfdec/swfdec_color.h>

G_BEGIN_DECLS

//typedef struct _SwfdecDraw SwfdecDraw;
typedef struct _SwfdecDrawClass SwfdecDrawClass;

#define SWFDEC_TYPE_DRAW                    (swfdec_draw_get_type())
#define SWFDEC_IS_DRAW(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DRAW))
#define SWFDEC_IS_DRAW_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DRAW))
#define SWFDEC_DRAW(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DRAW, SwfdecDraw))
#define SWFDEC_DRAW_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DRAW, SwfdecDrawClass))
#define SWFDEC_DRAW_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_DRAW, SwfdecDrawClass))

struct _SwfdecDraw
{
  GObject		object;

  /*< protected >*/
  gboolean		snap;		/* this drawing op does pixel snapping on the device grid */
  SwfdecRect		extents;	/* extents of path */
  cairo_path_t		path;		/* path to draw with this operation - in twips */
  cairo_path_t		end_path;     	/* end path to draw with this operation if morph operation */
};

struct _SwfdecDrawClass
{
  GObjectClass		object_class;

  /* morph this drawing operation using the given ratio */
  void			(* morph)		(SwfdecDraw *			dest,
						 SwfdecDraw *			source,
						 guint				ratio);
  /* paint the current drawing operation */
  void			(* paint)		(SwfdecDraw *			draw,
						 cairo_t *			cr,
						 const SwfdecColorTransform *	trans);
  /* compute extents of this drawing op for the given movie */
  void			(* compute_extents)   	(SwfdecDraw *			draw);
  /* check if the given coordinate is part of the area rendered to */
  gboolean		(* contains)		(SwfdecDraw *			draw,
						 cairo_t *			cr,
						 double				x,
						 double				y);
};

GType		swfdec_draw_get_type		(void);

SwfdecDraw *	swfdec_draw_morph		(SwfdecDraw *			draw,
						 guint				ratio);
SwfdecDraw *	swfdec_draw_copy		(SwfdecDraw *			draw);
void		swfdec_draw_paint		(SwfdecDraw *			draw, 
						 cairo_t *			cr,
						 const SwfdecColorTransform *	trans);
gboolean	swfdec_draw_contains		(SwfdecDraw *			draw,
						 double				x,
						 double				y);
void		swfdec_draw_recompute		(SwfdecDraw *			draw);

G_END_DECLS
#endif
