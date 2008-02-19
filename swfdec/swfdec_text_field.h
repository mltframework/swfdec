/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_TEXT_FIELD_H_
#define _SWFDEC_TEXT_FIELD_H_

#include <pango/pango.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_color.h>
#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_text_format.h>

G_BEGIN_DECLS

typedef struct _SwfdecTextField SwfdecTextField;
typedef struct _SwfdecTextFieldClass SwfdecTextFieldClass;

#define SWFDEC_TYPE_TEXT_FIELD                    (swfdec_text_field_get_type())
#define SWFDEC_IS_TEXT_FIELD(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_FIELD))
#define SWFDEC_IS_TEXT_FIELD_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_FIELD))
#define SWFDEC_TEXT_FIELD(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_FIELD, SwfdecTextField))
#define SWFDEC_TEXT_FIELD_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_FIELD, SwfdecTextFieldClass))

typedef enum {
  SWFDEC_AUTO_SIZE_NONE,
  SWFDEC_AUTO_SIZE_LEFT,
  SWFDEC_AUTO_SIZE_RIGHT,
  SWFDEC_AUTO_SIZE_CENTER
} SwfdecAutoSize;

struct _SwfdecTextField
{
  SwfdecGraphic		graphic;

  gboolean		html;

  gboolean		editable;
  gboolean		password;
  int			max_chars;
  gboolean		selectable;

  gboolean		embed_fonts;

  gboolean		word_wrap;
  gboolean		multiline;
  SwfdecAutoSize	auto_size;

  gboolean		border;
  gboolean		background;

  /* only to be passed to the movie object */
  char *		input;
  char *		variable;
  char *		font;
  guint			size;
  SwfdecColor		color;
  SwfdecTextAlign	align;
  guint			left_margin;
  guint			right_margin;
  guint			indent;
  int			leading;
};

struct _SwfdecTextFieldClass
{
  SwfdecGraphicClass	graphic_class;
};

GType			swfdec_text_field_get_type	(void);

int			tag_func_define_edit_text	(SwfdecSwfDecoder *	s,
							 guint			tag);

G_END_DECLS
#endif
