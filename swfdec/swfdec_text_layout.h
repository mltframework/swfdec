/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
 *                    2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TEXT_LAYOUT_H_
#define _SWFDEC_TEXT_LAYOUT_H_

#include <swfdec_rectangle.h>
#include <swfdec_renderer.h>
#include <swfdec_text_buffer.h>

G_BEGIN_DECLS

typedef struct _SwfdecTextLayout SwfdecTextLayout;
typedef struct _SwfdecTextLayoutClass SwfdecTextLayoutClass;

#define SWFDEC_TYPE_TEXT_LAYOUT                    (swfdec_text_layout_get_type())
#define SWFDEC_IS_TEXT_LAYOUT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_LAYOUT))
#define SWFDEC_IS_TEXT_LAYOUT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_LAYOUT))
#define SWFDEC_TEXT_LAYOUT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_LAYOUT, SwfdecTextLayout))
#define SWFDEC_TEXT_LAYOUT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_LAYOUT, SwfdecTextLayoutClass))
#define SWFDEC_TEXT_LAYOUT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEXT_LAYOUT, SwfdecTextLayoutClass))

struct _SwfdecTextLayout
{
  GObject		object;

  SwfdecTextBuffer *	text;		/* the text we render */
  /* properties */
  gboolean		word_wrap;	/* TRUE if we do word wrapping */
  guint			width;		/* width we use for alignment */
  gboolean		password;	/* TRUE if the text should be displayed as asterisks */
  double		scale;		/* scale factor in use */

  /* layout data */
  GSequence *		blocks;		/* ordered list of blocks */
};

struct _SwfdecTextLayoutClass
{
  GObjectClass		object_class;
};

GType			swfdec_text_layout_get_type		(void);

SwfdecTextLayout *	swfdec_text_layout_new			(SwfdecTextBuffer *	buffer);

void			swfdec_text_layout_set_wrap_width	(SwfdecTextLayout *	layout,
								 guint			width);
guint			swfdec_text_layout_get_wrap_width	(SwfdecTextLayout *	layout);
void			swfdec_text_layout_set_word_wrap	(SwfdecTextLayout *	layout,
								 gboolean		wrap);
gboolean		swfdec_text_layout_get_word_wrap	(SwfdecTextLayout *	layout);
void			swfdec_text_layout_set_password		(SwfdecTextLayout *	layout,
								 gboolean		password);
gboolean		swfdec_text_layout_get_password		(SwfdecTextLayout *	layout);
void			swfdec_text_layout_set_scale		(SwfdecTextLayout *	layout,
								 double			scale);
double			swfdec_text_layout_get_scale		(SwfdecTextLayout *	layout);
guint			swfdec_text_layout_get_width		(SwfdecTextLayout *	layout);
guint			swfdec_text_layout_get_height		(SwfdecTextLayout *	layout);

guint			swfdec_text_layout_get_n_rows		(SwfdecTextLayout *	layout);
guint			swfdec_text_layout_get_visible_rows	(SwfdecTextLayout *	layout,
								 guint			row,
								 guint			height);
guint			swfdec_text_layout_get_visible_rows_end	(SwfdecTextLayout *	layout,
								 guint			height);
void			swfdec_text_layout_get_ascent_descent	(SwfdecTextLayout *	layout,
								 int *			ascent,
								 int *			descent);

void			swfdec_text_layout_render		(SwfdecTextLayout *	layout,
								 cairo_t *		cr, 
								 const SwfdecColorTransform *ctrans,
								 guint			row,
								 guint			height,
								 SwfdecColor		focus);
void			swfdec_text_layout_query_position	(SwfdecTextLayout *	layout,
								 guint			row,
								 int			x,
								 int			y,
								 gsize *		index_,
								 gboolean *		hit,
								 int *			trailing);


G_END_DECLS
#endif
