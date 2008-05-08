/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_TEXT_FIELD_MOVIE_H_
#define _SWFDEC_TEXT_FIELD_MOVIE_H_

#include <swfdec/swfdec_actor.h>
#include <swfdec/swfdec_text_field.h>
#include <swfdec/swfdec_style_sheet.h>
#include <swfdec/swfdec_text_buffer.h>
#include <swfdec/swfdec_text_format.h>
#include <swfdec/swfdec_text_layout.h>

G_BEGIN_DECLS


typedef struct _SwfdecTextFieldMovie SwfdecTextFieldMovie;
typedef struct _SwfdecTextFieldMovieClass SwfdecTextFieldMovieClass;

#define SWFDEC_TYPE_TEXT_FIELD_MOVIE                    (swfdec_text_field_movie_get_type())
#define SWFDEC_IS_TEXT_FIELD_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_FIELD_MOVIE))
#define SWFDEC_IS_TEXT_FIELD_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_FIELD_MOVIE))
#define SWFDEC_TEXT_FIELD_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_FIELD_MOVIE, SwfdecTextFieldMovie))
#define SWFDEC_TEXT_FIELD_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_FIELD_MOVIE, SwfdecTextFieldMovieClass))

struct _SwfdecTextFieldMovie {
  SwfdecActor		actor;

  SwfdecRect		extents;	/* original extents (copied from graphic) */
  double		xscale;		/* scale movie => stage in x direction */
  double		yscale;		/* scale movie => stage in y direction */
  SwfdecRectangle	stage_rect;	/* these extents in stage coordinates */

  /* properties copied from textfield */
  gboolean		html;
  gboolean		editable;
  int			max_chars;
  gboolean		selectable;
  gboolean		embed_fonts;
  gboolean		word_wrap;
  gboolean		multiline;
  SwfdecAutoSize	auto_size;
  gboolean		border;
  gboolean		background;
 
  SwfdecTextBuffer *	text;		/* the text + formatting */
  gboolean		input_html;	/* whether orginal input was given as HTML */

  SwfdecTextLayout *	layout;		/* the layouted text */
  guint			layout_width;	/* text width in pixels */
  guint			layout_height;	/* text height in pixels */

  const char *		variable;

  SwfdecTextAttributes	default_attributes;

  gboolean		condense_white;

  SwfdecAsObject *	style_sheet;
  const char *		style_sheet_input; /* saved input, so it can be used to apply stylesheet again */

  gboolean		scroll_changed; /* if any of the scroll attributes have changed and we haven't fired the event yet */
  guint			changed;	/* number of onChanged events we have to emit */
  int			scroll;
  int			scroll_max;
  int			scroll_bottom;
  int			hscroll;
  int			hscroll_max;
  gboolean		mouse_wheel_enabled;

  const char *		restrict_;

  SwfdecColor		border_color;
  SwfdecColor		background_color;

  gboolean		mouse_pressed;
  guint			character_pressed;
};

struct _SwfdecTextFieldMovieClass {
  SwfdecActorClass	actor_class;
};

GType		swfdec_text_field_movie_get_type		(void);

void		swfdec_text_field_movie_set_text	(SwfdecTextFieldMovie *	movie,
							 const char *		str,
							 gboolean		html);
gboolean	swfdec_text_field_movie_auto_size	(SwfdecTextFieldMovie *	text);
void		swfdec_text_field_movie_update_scroll	(SwfdecTextFieldMovie *	text,
							 gboolean		check_limits);
const char *	swfdec_text_field_movie_get_text	(SwfdecTextFieldMovie *		text);
void		swfdec_text_field_movie_set_listen_variable (SwfdecTextFieldMovie *	text,
							 const char *			value);
void		swfdec_text_field_movie_set_listen_variable_text (SwfdecTextFieldMovie		*text,
							 const char *			value);
void		swfdec_text_field_movie_replace_text	(SwfdecTextFieldMovie *		text,
							 guint				start_index,
							 guint				end_index,
							 const char *			str);

/* implemented in swfdec_text_field_movie_as.c */
void		swfdec_text_field_movie_init_properties	(SwfdecAsContext *	cx);

/* implemented in swfdec_html_parser.c */
void		swfdec_text_field_movie_html_parse	(SwfdecTextFieldMovie *	text, 
							 const char *		str);
const char *	swfdec_text_field_movie_get_html_text	(SwfdecTextFieldMovie *		text);


G_END_DECLS
#endif
