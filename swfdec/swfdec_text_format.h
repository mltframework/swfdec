/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TEXT_FORMAT_H_
#define _SWFDEC_TEXT_FORMAT_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_array.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecTextFormat SwfdecTextFormat;
typedef struct _SwfdecTextFormatClass SwfdecTextFormatClass;

#define SWFDEC_TYPE_TEXT_FORMAT                    (swfdec_text_format_get_type())
#define SWFDEC_IS_TEXT_FORMAT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_FORMAT))
#define SWFDEC_IS_TEXT_FORMAT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_FORMAT))
#define SWFDEC_TEXT_FORMAT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_FORMAT, SwfdecTextFormat))
#define SWFDEC_TEXT_FORMAT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_FORMAT, SwfdecTextFormatClass))
#define SWFDEC_TEXT_FORMAT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEXT_FORMAT, SwfdecTextFormatClass))

typedef enum {
  SWFDEC_TEXT_ALIGN_LEFT,
  SWFDEC_TEXT_ALIGN_RIGHT,
  SWFDEC_TEXT_ALIGN_CENTER,
  SWFDEC_TEXT_ALIGN_JUSTIFY
} SwfdecTextAlign;

typedef enum {
  SWFDEC_TEXT_DISPLAY_NONE,
  SWFDEC_TEXT_DISPLAY_INLINE,
  SWFDEC_TEXT_DISPLAY_BLOCK,
} SwfdecTextDisplay;

struct _SwfdecTextFormat {
  SwfdecAsObject	object;

  SwfdecTextAlign	align;
  int			block_indent;
  gboolean		bold;
  gboolean		bullet;
  SwfdecColor		color;
  SwfdecTextDisplay	display;
  const char *		font;
  int			indent;
  gboolean		italic;
  gboolean		kerning;
  int			leading;
  int			left_margin;
  double		letter_spacing; // number or null
  int			right_margin;
  int			size;
  SwfdecAsArray *	tab_stops;
  const char *		target;
  gboolean		underline;
  const char *		url;

  int			values_set;
};

struct _SwfdecTextFormatClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_text_format_get_type	(void);

SwfdecAsObject * swfdec_text_format_new		(SwfdecAsContext *	context);
SwfdecAsObject * swfdec_text_format_new_no_properties (SwfdecAsContext *	context);
void		swfdec_text_format_set_defaults	(SwfdecTextFormat *	format);
SwfdecTextFormat * swfdec_text_format_copy	(const SwfdecTextFormat *copy_from);
void		swfdec_text_format_add		(SwfdecTextFormat *	format,
						 const SwfdecTextFormat *from);
gboolean	swfdec_text_format_equal	(const SwfdecTextFormat *a,
						 const SwfdecTextFormat *b);
gboolean	swfdec_text_format_equal_or_undefined	(const SwfdecTextFormat *a,
						 const SwfdecTextFormat *b);
void		swfdec_text_format_remove_different (SwfdecTextFormat *		format,
						 const SwfdecTextFormat *	from);
void		swfdec_text_format_init_properties (SwfdecAsContext *		cx);

G_END_DECLS
#endif
