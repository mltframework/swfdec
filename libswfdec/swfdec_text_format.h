/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *		 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TEXTFORMAT_H_
#define _SWFDEC_TEXTFORMAT_H_

#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_as_array.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecTextFormat SwfdecTextFormat;
typedef struct _SwfdecTextFormatClass SwfdecTextFormatClass;

#define SWFDEC_TYPE_TEXTFORMAT                    (swfdec_text_format_get_type())
#define SWFDEC_IS_TEXTFORMAT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXTFORMAT))
#define SWFDEC_IS_TEXTFORMAT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXTFORMAT))
#define SWFDEC_TEXTFORMAT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXTFORMAT, SwfdecTextFormat))
#define SWFDEC_TEXTFORMAT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXTFORMAT, SwfdecTextFormatClass))
#define SWFDEC_TEXTFORMAT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEXTFORMAT, SwfdecTextFormatClass))

typedef enum {
  SWFDEC_TEXT_ALIGN_UNDEFINED,
  SWFDEC_TEXT_ALIGN_LEFT,
  SWFDEC_TEXT_ALIGN_CENTER,
  SWFDEC_TEXT_ALIGN_RIGHT,
  SWFDEC_TEXT_ALIGN_JUSTIFY
} SwfdecTextAlign;

typedef enum {
  SWFDEC_TOGGLE_UNDEFINED = -1,
  SWFDEC_TOGGLE_DISABLED = 0,
  SWFDEC_TOGGLE_ENABLED = 1
} SwfdecToggle;

struct _SwfdecTextFormat {
  SwfdecAsObject	object;

  SwfdecTextAlign	align;
  double		blockIndent;
  SwfdecToggle		bold;
  SwfdecToggle		bullet;
  double		color;
  const char *		font;
  double		indent;
  SwfdecToggle		italic;
  SwfdecToggle		kerning;
  double		leading;
  double		leftMargin;
  SwfdecAsValue		letterSpacing;
  double		rightMargin;
  double		size;		// null?
  SwfdecAsArray		tabStops;
  const char *		target;
  SwfdecToggle		underline;
  const char *		url;
};

struct _SwfdecTextFormatClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_text_format_get_type	(void);

G_END_DECLS
#endif
