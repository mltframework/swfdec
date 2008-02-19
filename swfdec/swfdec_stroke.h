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

#include <swfdec/swfdec_pattern.h>

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
  SwfdecDraw		draw;

  guint			start_width;		/* width of line */
  SwfdecColor		start_color;		/* color to paint with */
  guint			end_width;		/* width of line */
  SwfdecColor		end_color;		/* color to paint with */
  /* Flash 8 */
  SwfdecPattern *	pattern;		/* pattern to use instead of color if not NULL */
  cairo_line_cap_t	start_cap;
  cairo_line_cap_t	end_cap;
  cairo_line_join_t	join;
  guint			miter_limit;		/* only set when join = MITER */
  gboolean		no_vscale;		/* don't scale line in vertical direction */
  gboolean		no_hscale;		/* don't scale line in horizontal direction */
  gboolean		no_close;		/* don't auto-close lines */
};

struct _SwfdecStrokeClass
{
  SwfdecDrawClass	draw_class;
};

GType		swfdec_stroke_get_type		(void);

SwfdecDraw *	swfdec_stroke_parse		(SwfdecBits *		bits,
						 SwfdecSwfDecoder *	dec);
SwfdecDraw *	swfdec_stroke_parse_rgba      	(SwfdecBits *		bits,
						 SwfdecSwfDecoder *	dec);
SwfdecDraw *	swfdec_stroke_parse_extended	(SwfdecBits *		bits,
						 SwfdecSwfDecoder *	dec);
SwfdecDraw *	swfdec_stroke_parse_morph    	(SwfdecBits *		bits,
						 SwfdecSwfDecoder *	dec);
SwfdecDraw *	swfdec_stroke_parse_morph_extended (SwfdecBits *	bits,
						 SwfdecSwfDecoder *	dec);


G_END_DECLS
#endif
