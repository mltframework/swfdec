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

#ifndef _SWFDEC_EDIT_TEXT_H_
#define _SWFDEC_EDIT_TEXT_H_

#include <pango/pango.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_color.h>
#include <libswfdec/swfdec_graphic.h>
#include <libswfdec/swfdec_player.h>

G_BEGIN_DECLS

typedef struct _SwfdecParagraph SwfdecParagraph; /* see swfdec_html_parser.c */
typedef struct _SwfdecEditText SwfdecEditText;
typedef struct _SwfdecEditTextClass SwfdecEditTextClass;

#define SWFDEC_TYPE_EDIT_TEXT                    (swfdec_edit_text_get_type())
#define SWFDEC_IS_EDIT_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_EDIT_TEXT))
#define SWFDEC_IS_EDIT_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_EDIT_TEXT))
#define SWFDEC_EDIT_TEXT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_EDIT_TEXT, SwfdecEditText))
#define SWFDEC_EDIT_TEXT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_EDIT_TEXT, SwfdecEditTextClass))

struct _SwfdecEditText
{
  SwfdecGraphic		graphic;

  /* text info */
  char *		text;		/* initial displayed text or NULL if none */
  gboolean		password;	/* if text is a password and should be displayed as '*' */
  guint		max_length;	/* maximum number of characters */
  gboolean		html;		/* text is pseudo-html */

  /* layout info */
  SwfdecFont *	  	font;		/* font or NULL for default */
  gboolean		wrap;
  gboolean		multiline;
  PangoAlignment	align;
  gboolean		justify;
  guint		indent;		/* first line indentation */
  int			spacing;	/* spacing between lines */
  /* visual info */
  SwfdecColor		color;		/* text color */
  gboolean		selectable;
  gboolean		border;		/* draw a border around the text field */
  guint		height;
  guint		left_margin;
  guint		right_margin;
  gboolean		autosize;	/* FIXME: implement */

  /* variable info */
  char *		variable;	/* full name of the variable in dot notation */
  gboolean		readonly;
};

struct _SwfdecEditTextClass
{
  SwfdecGraphicClass	graphic_class;
};

GType			swfdec_edit_text_get_type	(void);

int			tag_func_define_edit_text	(SwfdecSwfDecoder *	s,
							 guint			tag);

/* implemented in swfdec_html_parser.c */
SwfdecParagraph *	swfdec_paragraph_html_parse   	(SwfdecEditText *	text, 
							 const char *		str);
SwfdecParagraph *	swfdec_paragraph_text_parse	(SwfdecEditText *       text,
							 const char *		str);
void			swfdec_paragraph_free		(SwfdecParagraph *	paragraphs);
void			swfdec_edit_text_render		(SwfdecEditText *	text,
							 cairo_t *		cr,
							 const SwfdecParagraph *	paragraph,
							 const SwfdecColorTransform *	trans,
							 const SwfdecRect *	rect,
							 gboolean		fill);


G_END_DECLS
#endif
