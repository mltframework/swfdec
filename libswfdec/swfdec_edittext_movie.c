/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_edittext_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecEditTextMovie, swfdec_edit_text_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_edit_text_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, 
      &SWFDEC_GRAPHIC (SWFDEC_EDIT_TEXT_MOVIE (movie)->text)->extents);
}

static void
swfdec_edit_text_movie_render (SwfdecMovie *movie, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);

  if (!fill) {
    cairo_rectangle (cr, movie->extents.x0, movie->extents.y0,
	movie->extents.x1 - movie->extents.x0,
	movie->extents.y1 - movie->extents.y0);
    return;
  }
  if (text->paragraph == NULL)
    return;
  swfdec_edit_text_render (text->text, cr, text->paragraph, trans, inval, fill);
}

static void
swfdec_edit_text_movie_dispose (GObject *object)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (object);

  swfdec_edit_text_movie_set_text (text, NULL);

  G_OBJECT_CLASS (swfdec_edit_text_movie_parent_class)->dispose (object);
}

static void
swfdec_edit_text_movie_iterate (SwfdecMovie *movie)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);
  SwfdecAsObject *parent;
  const char *s;
  SwfdecAsValue val = { 0, };

  if (text->text->variable == NULL)
    return;

  parent = SWFDEC_AS_OBJECT (movie->parent);
  swfdec_as_context_eval (parent->context, parent, text->text->variable, &val);
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&val))
    return;

  s = swfdec_as_value_to_string (parent->context, &val);
  g_assert (s);
  if (text->str && g_str_equal (s, text->str))
    return;

  swfdec_edit_text_movie_set_text (text, s);
}

static void
swfdec_edit_text_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);
  SwfdecAsObject *parent;
  SwfdecAsValue val = { 0, };
  const char *s;

  if (text->text->variable == NULL)
    return;

  parent = SWFDEC_AS_OBJECT (movie->parent);
  swfdec_as_context_eval (parent->context, parent, text->text->variable, &val);
  if (!SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
    s = swfdec_as_value_to_string (parent->context, &val);
    g_assert (s);
    if (text->str && g_str_equal (s, text->str))
      return;

    swfdec_edit_text_movie_set_text (text, s);
    return;
  }

  SWFDEC_LOG ("setting variable %s to \"%s\"", text->text->variable, 
      text->str ? text->str : "");
  s = text->str ? swfdec_as_context_get_string (parent->context, text->str) : SWFDEC_AS_STR_EMPTY;
  SWFDEC_AS_VALUE_SET_STRING (&val, s);
  swfdec_as_context_eval_set (parent->context, parent, text->text->variable, &val);
}

static void
swfdec_edit_text_movie_class_init (SwfdecEditTextMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_edit_text_movie_dispose;

  movie_class->init_movie = swfdec_edit_text_movie_init_movie;
  movie_class->update_extents = swfdec_edit_text_movie_update_extents;
  movie_class->render = swfdec_edit_text_movie_render;
  movie_class->iterate_start = swfdec_edit_text_movie_iterate;
}

static void
swfdec_edit_text_movie_init (SwfdecEditTextMovie *movie)
{
}

void
swfdec_edit_text_movie_set_text (SwfdecEditTextMovie *movie, const char *str)
{
  g_return_if_fail (SWFDEC_IS_EDIT_TEXT_MOVIE (movie));

  if (movie->paragraph) {
    swfdec_paragraph_free (movie->paragraph);
  }
  g_free (movie->str);
  movie->str = g_strdup (str);
  if (movie->str) {
    if (movie->text->html)
      movie->paragraph = swfdec_paragraph_html_parse (movie->text, movie->str);
    else
      movie->paragraph = swfdec_paragraph_text_parse (movie->text, movie->str);
  } else {
    movie->paragraph = NULL;
  }
  swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
}

