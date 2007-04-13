/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_STROKE_H_
#define _SWFDEC_STROKE_H_

#include <libswfdec/swfdec_pattern.h>

G_BEGIN_DECLS

typedef struct _SwfdecStroke SwfdecStroke;
typedef struct _SwfdecStrokeClass SwfdecStrokeClass;

#define SWFDEC_TYPE_STROKE                    (swfdec_stroke_get_type())
#define SWFDEC_IS_STROKE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_STROKE))
#define SWFDEC_IS_STROKE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_STROKE))
#define SWFDEC_STROKE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_STROKE, SwfdecStroke))
#define SWFDEC_STROKE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_STROKE, SwfdecStrokeClass))
#define SWFDEC_STROKE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_STROKE, SwfdecStrokeClass))

struct _SwfdecStroke
{
  GObject		object;

  guint			start_width;		/* width of line */
  SwfdecColor		start_color;		/* color to paint with */
  guint			end_width;		/* width of line */
  SwfdecColor		end_color;		/* color to paint with */
};

struct _SwfdecStrokeClass
{
  GObjectClass		object_class;
};

GType		swfdec_stroke_get_type		(void);

SwfdecStroke *	swfdec_stroke_new		(guint			width,
						 SwfdecColor		color);

void		swfdec_stroke_paint		(SwfdecStroke *		stroke,
						 cairo_t *		cr,
						 const cairo_path_t *	path,
						 const SwfdecColorTransform *trans,
						 guint			ratio);

SwfdecStroke *	swfdec_stroke_parse		(SwfdecSwfDecoder *	dec,
						 gboolean		rgba);
SwfdecStroke *	swfdec_stroke_parse_extended	(SwfdecSwfDecoder *	dec,
						 SwfdecPattern *	fill_styles,
						 guint			n_fill_styles);
SwfdecStroke *	swfdec_stroke_parse_morph    	(SwfdecSwfDecoder *	dec);
SwfdecStroke *	swfdec_stroke_parse_morph_extended
						(SwfdecSwfDecoder *	dec,
						 SwfdecPattern *	fill_styles,
						 guint			n_fill_styles);


G_END_DECLS
#endif
