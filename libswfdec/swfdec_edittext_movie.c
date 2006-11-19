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
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"

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
  SwfdecPlayer *player;
  jsval val;
  const char *s;

  if (text->text->variable == NULL)
    return;

  player = SWFDEC_ROOT_MOVIE (movie->root)->player;
  if (movie->parent->jsobj == NULL) {
    swfdec_js_add_movie (movie->parent);
    if (movie->parent->jsobj == NULL)
      return;
  }
  val = swfdec_js_eval (player->jscx, movie->parent->jsobj, text->text->variable);
  if (JSVAL_IS_VOID (val))
    return;

  s = swfdec_js_to_string (player->jscx, val);
  if (!s && !text->str)
    return;
  if (s && text->str && g_str_equal (s, text->str))
    return;

  swfdec_edit_text_movie_set_text (text, s);
}

static void
swfdec_edit_text_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);
  SwfdecPlayer *player;
  JSObject *object;
  JSString *string;
  jsval val;

  if (text->text->variable == NULL)
    return;

  player = SWFDEC_ROOT_MOVIE (movie->root)->player;
  if (movie->parent->jsobj == NULL) {
    swfdec_js_add_movie (movie->parent);
    if (movie->parent->jsobj == NULL)
      return;
  }
  object = movie->parent->jsobj;
  if (text->text->variable_prefix) {
    val = swfdec_js_eval (player->jscx, object, text->text->variable_prefix);
    if (!JSVAL_IS_OBJECT (val))
      return;
    object = JSVAL_TO_OBJECT (val);
  }
  if (!JS_GetProperty (player->jscx, object, text->text->variable_name, &val))
    return;
  if (!JSVAL_IS_VOID (val)) {
    const char *s = swfdec_js_to_string (player->jscx, val);
    if (!s && !text->str)
      return;
    if (s && text->str && g_str_equal (s, text->str))
      return;

    swfdec_edit_text_movie_set_text (text, s);
    return;
  }

  SWFDEC_LOG ("setting variable %s (%s) to \"%s\"", text->text->variable_name,
      text->text->variable, text->str ? text->str : "");
  string = JS_NewStringCopyZ (player->jscx, text->str ? text->str : "");
  if (string == NULL)
    return;
  val = STRING_TO_JSVAL (string);
  JS_SetProperty (player->jscx, object, text->text->variable_name, &val);
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

