/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_FONT_H_
#define _SWFDEC_FONT_H_

#include <pango/pangocairo.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_character.h>

G_BEGIN_DECLS
//typedef struct _SwfdecFont SwfdecFont;
typedef struct _SwfdecFontEntry SwfdecFontEntry;
typedef struct _SwfdecFontClass SwfdecFontClass;

#define SWFDEC_TEXT_SCALE_FACTOR		(1024)

typedef enum {
  SWFDEC_LANGUAGE_NONE		= 0,
  SWFDEC_LANGUAGE_LATIN		= 1,
  SWFDEC_LANGUAGE_JAPANESE	= 2,
  SWFDEC_LANGUAGE_KOREAN	= 3,
  SWFDEC_LANGUAGE_CHINESE	= 4,
  SWFDEC_LANGUAGE_CHINESE_TRADITIONAL = 5
} SwfdecLanguage;

#define SWFDEC_TYPE_FONT                    (swfdec_font_get_type())
#define SWFDEC_IS_FONT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FONT))
#define SWFDEC_IS_FONT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FONT))
#define SWFDEC_FONT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FONT, SwfdecFont))
#define SWFDEC_FONT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FONT, SwfdecFontClass))

struct _SwfdecFontEntry {
  SwfdecDraw *		draw;		/* drawing operation to do or %NULL if none (ie space character) */
  gunichar		value;		/* UCS2 value of glyph */
  guint			advance;	/* advance when rendering character */
  SwfdecRect		extents;	/* bounding box of the font relative to the scale mode */
};

struct _SwfdecFont
{
  SwfdecCharacter	character;

  char *		name;		/* name of the font */
  char *		copyright;	/* copyright of the font */
  PangoFontDescription *desc;
  gboolean		bold;		/* font is bold */
  gboolean		italic;		/* font is italic */
  gboolean		small;		/* font is rendered at small sizes */
  GArray *		glyphs;		/* SwfdecFontEntry */
  guint			scale_factor;	/* size of a font in glyph entry */
  /* used when rendering TextFields */
  guint			ascent;		/* font ascent */
  guint			descent;	/* font descent */
  guint			leading;	/* */
};

struct _SwfdecFontClass
{
  SwfdecCharacterClass	character_class;
};

GType		swfdec_font_get_type		(void);

SwfdecDraw *	swfdec_font_get_glyph		(SwfdecFont *		font, 
						 guint			glyph);

int		tag_func_define_font_info	(SwfdecSwfDecoder *	s,
						 guint			tag);
int		tag_func_define_font		(SwfdecSwfDecoder *	s,
						 guint			tag);
int		tag_func_define_font_2		(SwfdecSwfDecoder *	s,
						 guint			tag);

int		tag_func_define_font_name	(SwfdecSwfDecoder *	s,
						 guint			tag);

G_END_DECLS
#endif
