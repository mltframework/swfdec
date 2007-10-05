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

#include <string.h>

#include "swfdec_edittext_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_text_format.h"
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
swfdec_edit_text_movie_generate_render_blocks (SwfdecEditTextMovie *text)
{
  SwfdecFormatIndex *findex;
  GSList *iter, *start;
  PangoAttribute *attr;
  const char *p, *end;
  int i, lines;
  guint start_index, end_index, findex_end_index;

  lines = 0;
  p = text->text_display;
  while (p != NULL && *p != '\0') {
    lines++;
    p = strchr (p, '\n');
    if (p != NULL) p++;
  }

  text->blocks = g_new0 (SwfdecTextRenderBlock, lines + 1);

  if (lines == 0)
    return;

  i = 0;
  start = text->formats;
  p = text->text_display;
  while (*p != '\0') {
    g_assert (i < lines);
    end = strchr (p, '\n');
    if (end == NULL)
      end = strchr (p, '\0');

    start_index = p - text->text_display;
    end_index = end - text->text_display;

    text->blocks[i].text = p;
    text->blocks[i].text_length = end - p;
    text->blocks[i].attrs = pango_attr_list_new ();

    while (start->next != NULL &&
	((SwfdecFormatIndex *)(start->next))->index < start_index) {
      start = start->next;
    }

    // things we only set on start of paragraph
    findex = start->data;
    switch (findex->format->align) {
      case SWFDEC_TEXT_ALIGN_LEFT:
	text->blocks[i].align = PANGO_ALIGN_LEFT;
	text->blocks[i].justify = FALSE;
	break;
      case SWFDEC_TEXT_ALIGN_RIGHT:
	text->blocks[i].align = PANGO_ALIGN_RIGHT;
	text->blocks[i].justify = FALSE;
	break;
      case SWFDEC_TEXT_ALIGN_CENTER:
	text->blocks[i].align = PANGO_ALIGN_CENTER;
	text->blocks[i].justify = FALSE;
	break;
      case SWFDEC_TEXT_ALIGN_JUSTIFY:
	text->blocks[i].align = PANGO_ALIGN_LEFT;
	text->blocks[i].justify = TRUE;
	break;
    }
    text->blocks[i].bullet = findex->format->bullet;
    text->blocks[i].indent = findex->format->indent;
    text->blocks[i].leading = findex->format->leading;
    text->blocks[i].block_indent = findex->format->block_indent;
    text->blocks[i].left_margin = findex->format->left_margin;
    text->blocks[i].right_margin = findex->format->right_margin;
    text->blocks[i].spacing = findex->format->letter_spacing;
    //PangoTabArray *	tabs;

    for (iter = start;
	iter != NULL && ((SwfdecFormatIndex *)iter)->index < end_index;
	iter = iter->next)
    {
      findex = iter->data;
      g_assert (findex != NULL);
      g_assert (SWFDEC_IS_TEXT_FORMAT (findex->format));

      findex_end_index = (iter->next != NULL ?
	  ((SwfdecFormatIndex *)(iter->next))->index :
	  strlen (text->text_display));

      g_print (":: %i - %i\n", findex->index, findex_end_index);

      attr = pango_attr_size_new_absolute (findex->format->size * 20 * PANGO_SCALE);
      attr->start_index = (findex->index < start_index ?
	  0 : findex->index - start_index);
      attr->end_index = (findex_end_index > end_index ?
	  end_index - start_index : findex_end_index - start_index);
      pango_attr_list_change (text->blocks[i].attrs, attr);
      g_print (":: %i-%i :: %i\n", attr->start_index, attr->end_index, findex->format->size);
    };

    p = end;
    if (*p != '\0') p++;
    i++;
  }

  g_print (":: %s\n", text->blocks[0].text);
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

  if (text->blocks == NULL)
    swfdec_edit_text_movie_generate_render_blocks (text);

  if (text->blocks[0].text == NULL)
    return;
  swfdec_edit_text_render (text->text, cr, text->blocks, trans, inval, fill);
}

void
swfdec_edit_text_movie_format_changed (SwfdecEditTextMovie *text)
{
  int i;

  if (text->blocks != NULL) {
    for (i = 0; text->blocks[i].text != NULL; i++) {
      // tabs
      pango_attr_list_unref (text->blocks[i].attrs);
    }
    g_free (text->blocks);
    text->blocks = NULL;
  }
}

static void
swfdec_edit_text_movie_dispose (GObject *object)
{
  int i;
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (object);

  if (text->blocks) {
    for (i = 0; text->blocks[i].text != NULL; i++) {
      if (text->blocks[i].attrs != NULL)
	pango_attr_list_unref (text->blocks[i].attrs);
      if (text->blocks[i].tabs != NULL)
	pango_tab_array_free (text->blocks[i].tabs);
    }
    g_free (text->blocks);
    text->blocks = NULL;
  }

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
  if (text->text_input == s)
    return;

  swfdec_edit_text_movie_set_text (text, s);
}

static void
swfdec_edit_text_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecEditTextMovie *text = SWFDEC_EDIT_TEXT_MOVIE (movie);
  SwfdecAsContext *cx;
  SwfdecAsObject *parent;
  SwfdecAsValue val = { 0, };
  const char *s;

  cx = SWFDEC_AS_OBJECT (movie)->context;

  // format
  text->format_new = SWFDEC_TEXT_FORMAT (swfdec_text_format_new (cx));
  swfdec_text_format_set_defaults (text->format_new);
  text->format_new->align = text->text->align;
  /*SwfdecColor		color;
  SwfdecTextAlign	align;
  guint			left_margin;
  guint			right_margin;
  guint			indent;
  int			spacing;*/

  // text
  if (text->text->text_input != NULL) {
    swfdec_edit_text_movie_set_text (text,
	swfdec_as_context_get_string (cx, text->text->text_input));
  }

  // variable
  if (text->text->variable != NULL)
  {
    text->variable = swfdec_as_context_get_string (cx, text->text->variable);

    parent = SWFDEC_AS_OBJECT (movie->parent);
    swfdec_as_context_eval (parent->context, parent, text->variable, &val);
    if (!SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
      s = swfdec_as_value_to_string (parent->context, &val);
      g_assert (s);
      if (text->text_input != s)
	swfdec_edit_text_movie_set_text (text, s);
    } else {
      SWFDEC_LOG ("setting variable %s to \"%s\"", text->variable,
	  text->text_input ? text->text_input : "");
      s = text->text_input ? swfdec_as_context_get_string (parent->context,
	  text->text_input) : SWFDEC_AS_STR_EMPTY;
      SWFDEC_AS_VALUE_SET_STRING (&val, s);
      swfdec_as_context_eval_set (parent->context, parent, text->variable,
	  &val);
    }
  }
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
swfdec_edit_text_movie_init (SwfdecEditTextMovie *text)
{
}

void
swfdec_edit_text_movie_set_text (SwfdecEditTextMovie *text, const char *str)
{
  SwfdecFormatIndex *block;
  GSList *iter;

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT_MOVIE (text));

  g_print ("SET_TEXT!\n");

  text->text_input = str;

  // remove old formatting info
  iter = text->formats;
  while (iter) {
    g_free (iter->data);
    iter = g_slist_next (iter);
  }
  g_slist_free (text->formats);
  text->formats = NULL;

  // add the default style
  block = g_new (SwfdecFormatIndex, 1);
  block->index = 0;
  g_assert (SWFDEC_IS_TEXT_FORMAT (text->format_new));
  block->format = text->format_new;
  text->formats = g_slist_prepend (text->formats, block);

  //if (text->text->html == FALSE) {
    text->text_display = str;
  /*} else {
    swfdec_edit_text_text_parse_html (str, &text->text_display,
	&text->formats);
  }*/

  swfdec_edit_text_movie_generate_render_blocks (text);

  swfdec_movie_invalidate (SWFDEC_MOVIE (text));
}
