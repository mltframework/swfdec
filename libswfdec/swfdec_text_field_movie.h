/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
 *                    Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TEXT_FIELD_MOVIE_H_
#define _SWFDEC_TEXT_FIELD_MOVIE_H_

#include <libswfdec/swfdec_movie.h>
#include <libswfdec/swfdec_edittext.h>
#include <libswfdec/swfdec_style_sheet.h>
#include <libswfdec/swfdec_text_format.h>

G_BEGIN_DECLS


typedef struct _SwfdecTextFieldMovie SwfdecTextFieldMovie;
typedef struct _SwfdecTextFieldMovieClass SwfdecTextFieldMovieClass;

#define SWFDEC_TYPE_TEXT_FIELD_MOVIE                    (swfdec_text_field_movie_get_type())
#define SWFDEC_IS_TEXT_FIELD_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_FIELD_MOVIE))
#define SWFDEC_IS_TEXT_FIELD_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_FIELD_MOVIE))
#define SWFDEC_TEXT_FIELD_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_FIELD_MOVIE, SwfdecTextFieldMovie))
#define SWFDEC_TEXT_FIELD_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_FIELD_MOVIE, SwfdecTextFieldMovieClass))

typedef struct {
  guint			index;
  SwfdecTextFormat *	format;
} SwfdecFormatIndex;

typedef enum {
  SWFDEC_ANTI_ALIAS_TYPE_NORMAL,
  SWFDEC_ANTI_ALIAS_TYPE_ADVANCED
} SwfdecAntiAliasType;

typedef enum {
  SWFDEC_GRID_FIT_TYPE_NONE,
  SWFDEC_GRID_FIT_TYPE_PIXEL,
  SWFDEC_GRID_FIT_TYPE_SUBPIXEL
} SwfdecGridFitType;

struct _SwfdecTextFieldMovie {
  SwfdecMovie		movie;

  SwfdecTextField *	text;		/* the text_field object we render */

  const char *		text_input;
  const char *		text_display;

  const char *		variable;

  SwfdecTextFormat *	format_new;
  GSList *		formats;

  SwfdecAntiAliasType	anti_alias_type;
  gboolean		background_fill;
  guint			border_color;
  gboolean		condense_white;
  gboolean		embed_fonts;
  SwfdecGridFitType	grid_fit_type;
  guint			hscroll;
  gboolean		mouse_wheel_enabled;
  const char *		restrict_;
  guint			scroll;
  int			sharpness;
  SwfdecStyleSheet *	style_sheet;
  int			thickness;

  /* for rendering */
  SwfdecParagraph *	paragraphs;
};

struct _SwfdecTextFieldMovieClass {
  SwfdecMovieClass	movie_class;
};

GType		swfdec_text_field_movie_get_type		(void);

void		swfdec_text_field_movie_set_text		(SwfdecTextFieldMovie *	movie,
							 const char *		str,
							 gboolean		html);
void		swfdec_text_field_movie_format_changed	(SwfdecTextFieldMovie *	text);
void		swfdec_text_field_movie_set_text_format	(SwfdecTextFieldMovie *	text,
							 SwfdecTextFormat *	format,
							 guint			start_index,
							 guint			end_index);

/* implemented in swfdec_html_parser.c */
void		swfdec_text_field_movie_html_parse	(SwfdecTextFieldMovie *	text, 
							 const char *		str);

G_END_DECLS
#endif
