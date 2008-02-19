/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_TEXT_H_
#define _SWFDEC_TEXT_H_

#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_color.h>

G_BEGIN_DECLS
//typedef struct _SwfdecText SwfdecText;
typedef struct _SwfdecTextClass SwfdecTextClass;

typedef struct _SwfdecTextGlyph SwfdecTextGlyph;

#define SWFDEC_TYPE_TEXT                    (swfdec_text_get_type())
#define SWFDEC_IS_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT))
#define SWFDEC_IS_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT))
#define SWFDEC_TEXT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT, SwfdecText))
#define SWFDEC_TEXT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT, SwfdecTextClass))

struct _SwfdecTextGlyph {
  int			x;
  int			y;
  int			glyph;
  SwfdecFont *		font;
  int			height;
  SwfdecColor		color;
};

struct _SwfdecText {
  SwfdecGraphic		graphic;

  GArray *		glyphs;
  cairo_matrix_t	transform;
  cairo_matrix_t	transform_inverse;
};

struct _SwfdecTextClass {
  SwfdecGraphicClass	graphic_class;
};

GType swfdec_text_get_type (void);

int tag_func_define_text (SwfdecSwfDecoder * s, guint tag);


G_END_DECLS
#endif
