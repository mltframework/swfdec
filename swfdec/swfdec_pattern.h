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

#ifndef _SWFDEC_PATTERN_H_
#define _SWFDEC_PATTERN_H_

#include <glib-object.h>
#include <cairo.h>
#include <swfdec/swfdec_draw.h>
#include <swfdec/swfdec_swf_decoder.h>

G_BEGIN_DECLS

typedef struct _SwfdecPattern SwfdecPattern;
typedef struct _SwfdecPatternClass SwfdecPatternClass;

#define SWFDEC_TYPE_PATTERN                    (swfdec_pattern_get_type())
#define SWFDEC_IS_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_PATTERN))
#define SWFDEC_IS_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_PATTERN))
#define SWFDEC_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_PATTERN, SwfdecPattern))
#define SWFDEC_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_PATTERN, SwfdecPatternClass))
#define SWFDEC_PATTERN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_PATTERN, SwfdecPatternClass))

struct _SwfdecPattern
{
  SwfdecDraw		draw;

  cairo_matrix_t	transform;		/* user-to-pattern transform */
  cairo_matrix_t	start_transform;	/* start transform */
  cairo_matrix_t	end_transform;		/* end transform (if morph) */
};

struct _SwfdecPatternClass
{
  SwfdecDrawClass	draw_class;

  /* create a cairo pattern for the given values */
  cairo_pattern_t *	(* get_pattern)		(SwfdecPattern *		pattern,
						 const SwfdecColorTransform *	trans);
};

GType		swfdec_pattern_get_type		(void);

SwfdecPattern *	swfdec_pattern_new_color	(SwfdecColor			color);
SwfdecDraw *	swfdec_pattern_parse		(SwfdecBits *			bits,
						 SwfdecSwfDecoder *		dec);
SwfdecDraw *	swfdec_pattern_parse_rgba     	(SwfdecBits *			bits,
						 SwfdecSwfDecoder *		dec);
SwfdecDraw *	swfdec_pattern_parse_morph    	(SwfdecBits *			bits,
						 SwfdecSwfDecoder *		dec);

cairo_pattern_t *swfdec_pattern_get_pattern	(SwfdecPattern *		pattern, 
						 const SwfdecColorTransform *	trans);

/* debug */
char *		swfdec_pattern_to_string	(SwfdecPattern *		pattern);

G_END_DECLS
#endif
