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
#include <libswfdec/swfdec_text_format.h>

G_BEGIN_DECLS

typedef struct _SwfdecEditText SwfdecEditText;
typedef struct _SwfdecEditTextClass SwfdecEditTextClass;

#define SWFDEC_TYPE_EDIT_TEXT                    (swfdec_edit_text_get_type())
#define SWFDEC_IS_EDIT_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_EDIT_TEXT))
#define SWFDEC_IS_EDIT_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_EDIT_TEXT))
#define SWFDEC_EDIT_TEXT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_EDIT_TEXT, SwfdecEditText))
#define SWFDEC_EDIT_TEXT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_EDIT_TEXT, SwfdecEditTextClass))

typedef struct {
  const char		*text;
  int			text_length;

  gboolean		bullet;
  int			indent;
  int			leading;
  int			block_indent;
  int			left_margin;
  int			right_margin;
  int			spacing;
  PangoTabArray *	tabs;

  PangoAttrList *	attrs;
  PangoAlignment	align;
  gboolean		justify;
} SwfdecTextRenderBlock;

typedef enum {
  SWFDEC_AUTO_SIZE_NONE,
  SWFDEC_AUTO_SIZE_LEFT,
  SWFDEC_AUTO_SIZE_CENTER,
  SWFDEC_AUTO_SIZE_RIGHT
} SwfdecAutoSize;

struct _SwfdecEditText
{
  SwfdecGraphic		graphic;

  gboolean		html;

  gboolean		input;
  gboolean		password;
  guint			max_length;
  gboolean		selectable;

  SwfdecFont *	  	font;

  gboolean		wrap;
  gboolean		multiline;
  SwfdecAutoSize	auto_size;

  gboolean		border;
  guint			height;

  /* only to be passed to the movie object */
  char *		text_input;
  char *		variable;
  SwfdecColor		color;
  SwfdecTextAlign	align;
  guint			left_margin;
  guint			right_margin;
  guint			indent;
  int			spacing;
};

struct _SwfdecEditTextClass
{
  SwfdecGraphicClass	graphic_class;
};

GType			swfdec_edit_text_get_type	(void);

int			tag_func_define_edit_text	(SwfdecSwfDecoder *	s,
							 guint			tag);

void			swfdec_edit_text_render		(SwfdecEditText *	text,
							 cairo_t *		cr,
							 const SwfdecTextRenderBlock *	blocks,
							 const SwfdecColorTransform *	trans,
							 const SwfdecRect *	rect,
							 gboolean		fill);

G_END_DECLS
#endif
