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
#include <libswfdec/swfdec_swf_decoder.h>
#include <libswfdec/swfdec_color.h>

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
  GObject		object;

  cairo_matrix_t	start_transform;	/* user-to-pattern transform */
  cairo_matrix_t	end_transform;		/* user-to-pattern transform */
};

struct _SwfdecPatternClass
{
  GObjectClass		object_class;

  /* create a cairo pattern for the given values */
  cairo_pattern_t *	(* get_pattern)		(SwfdecPattern *		pattern,
						 const SwfdecColorTransform *	trans,
						 guint				ratio);
};

GType		swfdec_pattern_get_type		(void);

SwfdecPattern *	swfdec_pattern_new_color	(SwfdecColor			color);
SwfdecPattern *	swfdec_pattern_parse		(SwfdecSwfDecoder *		dec,
						 gboolean			rgba);
SwfdecPattern *	swfdec_pattern_parse_morph    	(SwfdecSwfDecoder *		dec);

void		swfdec_pattern_paint		(SwfdecPattern *		pattern, 
						 cairo_t *			cr,
						 const cairo_path_t *		path,
						 const SwfdecColorTransform *	trans,
						 guint				ratio);
cairo_pattern_t *swfdec_pattern_get_pattern	(SwfdecPattern *		pattern, 
						 const SwfdecColorTransform *	trans,
						 guint				ratio);
void		swfdec_pattern_get_path_extents (SwfdecPattern *		pattern,
						 const cairo_path_t *		path,
						 SwfdecRect *			extents);

/* debug */
char *		swfdec_pattern_to_string	(SwfdecPattern *		pattern);

G_END_DECLS
#endif
