/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_TEXT_ATTRIBUTES_H_
#define _SWFDEC_TEXT_ATTRIBUTES_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_array.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecTextAttributes SwfdecTextAttributes;
typedef struct _SwfdecTextAttributesClass SwfdecTextAttributesClass;

#define SWFDEC_TYPE_TEXT_ATTRIBUTES                    (swfdec_text_attributes_get_type())
#define SWFDEC_IS_TEXT_ATTRIBUTES(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_ATTRIBUTES))
#define SWFDEC_IS_TEXT_ATTRIBUTES_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_ATTRIBUTES))
#define SWFDEC_TEXT_ATTRIBUTES(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_ATTRIBUTES, SwfdecTextAttributes))
#define SWFDEC_TEXT_ATTRIBUTES_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_ATTRIBUTES, SwfdecTextAttributesClass))
#define SWFDEC_TEXT_ATTRIBUTES_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEXT_ATTRIBUTES, SwfdecTextAttributesClass))

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

typedef enum {
  SWFDEC_TEXT_ATTRIBUTE_ALIGN = 0,
  SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT,
  SWFDEC_TEXT_ATTRIBUTE_BOLD,
  SWFDEC_TEXT_ATTRIBUTE_BULLET,
  SWFDEC_TEXT_ATTRIBUTE_COLOR,
  SWFDEC_TEXT_ATTRIBUTE_DISPLAY,
  SWFDEC_TEXT_ATTRIBUTE_FONT,
  SWFDEC_TEXT_ATTRIBUTE_INDENT,
  SWFDEC_TEXT_ATTRIBUTE_ITALIC,
  SWFDEC_TEXT_ATTRIBUTE_KERNING,
  SWFDEC_TEXT_ATTRIBUTE_LEADING,
  SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN,
  SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING,
  SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN,
  SWFDEC_TEXT_ATTRIBUTE_SIZE,
  SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS,
  SWFDEC_TEXT_ATTRIBUTE_TARGET,
  SWFDEC_TEXT_ATTRIBUTE_UNDERLINE,
  SWFDEC_TEXT_ATTRIBUTE_URL,
} SwfdecTextAttribute;

#define SWFDEC_TEXT_ATTRIBUTE_SET(flags, attribute) ((flags) |= 1 << (attribute))
#define SWFDEC_TEXT_ATTRIBUTE_UNSET(flags, attribute) ((flags) &= ~(1 << (attribute)))
#define SWFDEC_TEXT_ATTRIBUTE_IS_SET(flags, attribute) ((flags) & (1 << (attribute)) ? TRUE : FALSE)

struct _SwfdecTextAttributes {
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
  guint *		tab_stops;
  gsize			n_tab_stops;
  const char *		target;
  gboolean		underline;
  const char *		url;
};


void			swfdec_text_attributes_reset	(SwfdecTextAttributes *		attr);
void			swfdec_text_attributes_copy	(SwfdecTextAttributes *		attr,
							 const SwfdecTextAttributes *	from,
							 guint				flags);
void			swfdec_text_attributes_mark	(SwfdecTextAttributes *		attr);

guint			swfdec_text_attributes_diff	(const SwfdecTextAttributes *	a,
							 const SwfdecTextAttributes *	b);


G_END_DECLS
#endif
