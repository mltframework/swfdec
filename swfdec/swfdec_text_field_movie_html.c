/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include <stdlib.h>
#include <string.h>

#include "swfdec_text_field_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_style_sheet.h"
#include "swfdec_xml.h"
#include "swfdec_debug.h"

/*
 * Parsing
 */
typedef struct {
  const char *		name;
  int			name_length;
  guint			index;
  guint			end_index;
  SwfdecTextFormat *	format;
} ParserTag;

typedef struct {
  SwfdecAsContext *	cx;
  gboolean		multiline;
  gboolean		condense_white;
  SwfdecStyleSheet *	style_sheet;
  SwfdecTextBuffer *	text;
  GSList *		tags_open;
  GSList *		tags_closed;
} ParserData;

static void
swfdec_text_field_movie_html_parse_close_tag (ParserData *data, ParserTag *tag,
    gboolean end)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (tag != NULL);

  if (data->multiline && !end &&
      ((tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1)) ||
       (tag->name_length == 2 && !g_strncasecmp (tag->name, "li", 2))))
  {
    GSList *iter;

    for (iter = data->tags_closed; iter != NULL; iter = iter->next) {
      ParserTag *f = iter->data;
      if (f->end_index < tag->index)
	break;
      if (f->name_length == 4 && !g_strncasecmp (f->name, "font", 4)) {
	ParserTag *n = g_new0 (ParserTag, 1);
	n->name = f->name;
	n->name_length = f->name_length;
	n->index = swfdec_text_buffer_get_length (data->text);
	n->end_index = n->index + 1;
	if (f->format != NULL) {
	  n->format = swfdec_text_format_copy (f->format);
	} else {
	  n->format = NULL;
	}
	data->tags_closed = g_slist_prepend (data->tags_closed, n);
	break;
      }
    }
    swfdec_text_buffer_append_text (data->text, "\n");
  }

  tag->end_index = swfdec_text_buffer_get_length (data->text);

  data->tags_open = g_slist_remove (data->tags_open, tag);
  data->tags_closed = g_slist_prepend (data->tags_closed, tag);
}

static const char *
swfdec_text_field_movie_html_parse_comment (ParserData *data, const char *p)
{
  const char *end;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (p != NULL, NULL);
  g_return_val_if_fail (strncmp (p, "<!--", strlen ("<!--")) == 0, NULL);

  end = strstr (p + strlen ("<!--"), "-->");
  if (end != NULL)
    end += strlen("-->");

  // return NULL if no end found
  return end;
}

static void
swfdec_text_field_movie_html_tag_set_attribute (ParserData *data,
    ParserTag *tag, const char *name, int name_length, const char *value,
    int value_length)
{
  SwfdecAsValue val;
  SwfdecAsObject *object;

  g_return_if_fail (data != NULL);
  g_return_if_fail (tag != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (name_length >= 0);
  g_return_if_fail (value != NULL);
  g_return_if_fail (value_length >= 0);

  if (!tag->format)
    return;

  object = SWFDEC_AS_OBJECT (tag->format);
  SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (
	object->context, g_strndup (value, value_length)));

  if (tag->name_length == 10 && !g_strncasecmp (tag->name, "textformat", 10))
  {
    if (name_length == 10 && !g_strncasecmp (name, "leftmargin", 10))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_leftMargin, &val);
    }
    else if (name_length == 11 && !g_strncasecmp (name, "rightmargin", 11))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_rightMargin, &val);
    }
    else if (name_length == 6 && !g_strncasecmp (name, "indent", 6))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_indent, &val);
    }
    else if (name_length == 11 && !g_strncasecmp (name, "blockindent", 11))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_blockIndent, &val);
    }
    else if (name_length == 8 && !g_strncasecmp (name, "tabstops", 8))
    {
      // FIXME
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_tabStops, &val);
    }
  }
  else if (tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1))
  {
    if (name_length == 5 && !g_strncasecmp (name, "align", 5))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_align, &val);
    }
  }
  else if (tag->name_length == 4 && !g_strncasecmp (tag->name, "font", 4))
  {
    if (name_length == 4 && !g_strncasecmp (name, "face", 4))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_font, &val);
    }
    else if (name_length == 4 && !g_strncasecmp (name, "size", 4))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_size, &val);
    }
    else if (name_length == 5 && !g_strncasecmp (name, "color", 5))
    {
      SwfdecAsValue val_number;

      if (value_length != 7 || *value != '#') {
	SWFDEC_AS_VALUE_SET_NUMBER (&val_number, 0);
      } else {
	int number;
	char *tail;

	number = g_ascii_strtoll (value + 1, &tail, 16);
	if (tail != value + 7)
	  number = 0;
	SWFDEC_AS_VALUE_SET_NUMBER (&val_number, number);
      }

      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_color, &val_number);
    }
    else if (name_length == 13 && !g_strncasecmp (name, "letterspacing", 13))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_letterSpacing,
	  &val);
    }
    // special case: Don't parse kerning
  }
  else if (tag->name_length == 1 && !g_strncasecmp (tag->name, "a", 1))
  {
    if (name_length == 4 && !g_strncasecmp (name, "href", 4))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_url, &val);
    }
    else if (name_length == 6 && !g_strncasecmp (name, "target", 6))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_target, &val);
    }
  }

  if (data->style_sheet &&
      ((tag->name_length == 2 && !g_strncasecmp (tag->name, "li", 2)) ||
      (tag->name_length == 4 && !g_strncasecmp (tag->name, "span", 4)) ||
      (tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1))))
  {
    if (name_length == 5 && !g_strncasecmp (name, "class", 5)) {
      SwfdecTextFormat *format = swfdec_style_sheet_get_class_format (
	  data->style_sheet, swfdec_as_context_give_string (data->cx,
	      g_strndup (value, value_length)));
      if (format != NULL)
	swfdec_text_format_add (tag->format, format);
    }
  }
}

static const char *
swfdec_text_field_movie_html_parse_attribute (ParserData *data, ParserTag *tag,
    const char *p)
{
  const char *end, *name, *value;
  int name_length, value_length;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (tag != NULL, NULL);
  g_return_val_if_fail ((*p != '>' && *p != '\0'), NULL);

  end = p + strcspn (p, "=> \r\n\t");
  if (end - p <= 0)
    return NULL; // Correct?

  name = p;
  name_length = end - p;

  p = end + strspn (end, " \r\n\t");
  if (*p != '=')
    return NULL; // FIXME: Correct?
  p = p + 1;
  p = p + strspn (p, " \r\n\t");

  if (*p != '"' && *p != '\'')
    return NULL; // FIXME: Correct?

  end = p + 1;
  do {
    end = strchr (end, *p);
  } while (end != NULL && *(end - 1) == '\\');

  if (end == NULL)
    return NULL; // FIXME: Correct?

  value = p + 1;
  value_length = end - (p + 1);

  if (tag != NULL) {
    swfdec_text_field_movie_html_tag_set_attribute (data, tag, name,
	name_length, value, value_length);
  }

  g_return_val_if_fail (end + 1 > p, NULL);

  return end + 1;
}

static const char *
swfdec_text_field_movie_html_parse_tag (ParserData *data, const char *p)
{
  ParserTag *tag;
  const char *name, *end;
  int name_length;
  gboolean close;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (p != NULL, NULL);
  g_return_val_if_fail (*p == '<', NULL);

  p++;

  // closing tag or opening tag?
  if (*p == '/') {
    close = TRUE;
    p++;
  } else {
    close = FALSE;
  }

  // find the end of the name
  end = p + strcspn (p, "> \r\n\t");

  if (*end == '\0')
    return NULL;

  // don't count trailing / as part of the name if it's followed by >
  // we still act like it's a normal opening tag even if it has /
  if (*end == '>' && *(end - 1) == '/')
    end = end - 1;

  if (end == p) // empty name
    return NULL;

  name = p;
  name_length = end - p;

  if (close)
  {
    if (name_length == 1 && !g_strncasecmp (name, "p", 1)) {
      GSList *iter, *found;

      found = NULL;
      iter = data->tags_open;
      while (iter != NULL) {
	tag = iter->data;
	if (tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1))
	  found = iter;
	iter = iter->next;
      }

      if (found != NULL) {
	iter = data->tags_open;
	while (iter != found) {
	  tag = iter->data;
	  iter = iter->next;
	  if ((tag->name_length == 2 && !g_strncasecmp (tag->name, "li", 2)) ||
	      (tag->name_length == 10 &&
	       !g_strncasecmp (tag->name, "textformat", 10)))
	    continue;
	  swfdec_text_field_movie_html_parse_close_tag (data, tag, TRUE);
	}
	if (data->multiline)
	  swfdec_text_buffer_append_text (data->text, "\n");
	swfdec_text_field_movie_html_parse_close_tag (data, found->data,
	    TRUE);
      }
    } else {
      if (data->tags_open != NULL) {
	tag = data->tags_open->data;
	if (name_length == tag->name_length &&
	    !g_strncasecmp (name, tag->name, name_length))
	  swfdec_text_field_movie_html_parse_close_tag (data, tag, FALSE);
      }
    }

    end = strchr (end, '>');
    if (end != NULL)
      end += 1;
  }
  else
  {
    SwfdecAsObject *object;
    SwfdecAsValue val;

    if (name_length == 3 && !g_strncasecmp (name, "tab", 3))
      swfdec_text_buffer_append_text (data->text, "\t");

    if (data->multiline) {
      if (name_length == 2 && !g_strncasecmp (name, "br", 2))
      {
	swfdec_text_buffer_append_text (data->text, "\n");
      }
      else if (name_length == 2 && !g_strncasecmp (name, "li", 1))
      {
	gsize length = swfdec_text_buffer_get_length (data->text);
	const char *s = swfdec_text_buffer_get_text (data->text);
	if (length > 0 && s[length-1] != '\n' && s[length-1] != '\r')
	  swfdec_text_buffer_append_text (data->text, "\n");
      }
    }

    tag = g_new0 (ParserTag, 1);
    tag->name = name;
    tag->name_length = name_length;
    tag->format = SWFDEC_TEXT_FORMAT (swfdec_text_format_new (data->cx));
    tag->index = swfdec_text_buffer_get_length (data->text);

    data->tags_open = g_slist_prepend (data->tags_open, tag);

    // set format based on tag
    if (tag->format != NULL) {
      object = SWFDEC_AS_OBJECT (tag->format);
      SWFDEC_AS_VALUE_SET_BOOLEAN (&val, TRUE);

      if (tag->name_length == 2 && !g_strncasecmp (tag->name, "li", 2)) {
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_bullet, &val);
      } else if (tag->name_length == 1 && !g_strncasecmp (tag->name, "b", 1)) {
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_bold, &val);
      } else if (tag->name_length == 1 && !g_strncasecmp (tag->name, "i", 1)) {
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_italic, &val);
      } else if (tag->name_length == 1 && !g_strncasecmp (tag->name, "u", 1)) {
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_underline, &val);
      }
      else if (tag->name_length == 3 && !g_strncasecmp (tag->name, "img", 3))
      {
	SWFDEC_FIXME ("IMG tag support for TextField's HTML input missing");
      }
    }

    if (data->style_sheet &&
	((tag->name_length == 2 && !g_strncasecmp (tag->name, "li", 2)) ||
	(tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1)))) {
      SwfdecTextFormat *format = swfdec_style_sheet_get_tag_format (
	  data->style_sheet, swfdec_as_context_give_string (data->cx,
	      g_strndup (tag->name, tag->name_length)));
      if (format != NULL)
	swfdec_text_format_add (tag->format, format);
    }

    // parse attributes
    end = end + strspn (end, " \r\n\t");
    while (*end != '\0' && *end != '>' && (*end != '/' || *(end + 1) != '>')) {
      end = swfdec_text_field_movie_html_parse_attribute (data, tag, end);
      if (end == NULL)
	break;
      end = end + strspn (end, " \r\n\t");
    }
    if (end != NULL) {
      if (*end == '/')
	end += 1;
      if (*end == '>')
	end += 1;
    }
  }

  return end;
}

static const char *
swfdec_text_field_movie_html_parse_text (ParserData *data, const char *p)
{
  const char *end;
  char *unescaped;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (p != NULL, NULL);
  g_return_val_if_fail (*p != '\0' && *p != '<', NULL);

  // condense the space with previous text also, if version >= 8
  if (data->condense_white && data->cx->version >= 8) {
    gsize length = swfdec_text_buffer_get_length (data->text);
    const char *s = swfdec_text_buffer_get_text (data->text);
    if (length == 0 || g_ascii_isspace (s[length - 1]))
      p += strspn (p, " \n\r\t");
  }

  // get the text
  // if condense_white: all whitespace blocks are converted to a single space
  while (*p != '\0' && *p != '<') {
    if (data->condense_white) {
      end = p + strcspn (p, "< \n\r\t");
    } else {
      end = strchr (p, '<');
      if (end == NULL)
	end = strchr (p, '\0');
    }

    unescaped = swfdec_xml_unescape_len (data->cx, p, end - p, TRUE);
    swfdec_text_buffer_append_text (data->text, unescaped);
    g_free (unescaped);

    if (data->condense_white && g_ascii_isspace (*end)) {
      swfdec_text_buffer_append_text (data->text, " ");
      p = end + strspn (end, " \n\r\t");
    } else {
      p = end;
    }
  }

  return p;
}

void
swfdec_text_field_movie_html_parse (SwfdecTextFieldMovie *text, const char *str)
{
  ParserData data;
  const char *p;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (str != NULL);

  data.cx = SWFDEC_AS_OBJECT (text)->context;
  data.multiline = (data.cx->version < 7 || text->multiline);
  data.condense_white = text->condense_white;
  if (text->style_sheet != NULL && SWFDEC_IS_STYLESHEET (text->style_sheet)) {
    data.style_sheet = SWFDEC_STYLESHEET (text->style_sheet);
  } else {
    data.style_sheet = NULL;
  }
  data.text = text->text;
  data.tags_open = NULL;
  data.tags_closed = NULL;

  p = str;
  while (p != NULL && *p != '\0') {
    if (*p == '<') {
      if (strncmp (p + 1, "!--", strlen ("!--")) == 0) {
	p = swfdec_text_field_movie_html_parse_comment (&data, p);
      } else {
	p = swfdec_text_field_movie_html_parse_tag (&data, p);
      }
    } else {
      p = swfdec_text_field_movie_html_parse_text (&data, p);
    }
  }

  // close remaining tags
  while (data.tags_open != NULL) {
    /* yes, this really appends to the default format */
    swfdec_text_attributes_copy (&text->default_attributes,
	&((ParserTag *)data.tags_open->data)->format->attr,
	((ParserTag *)data.tags_open->data)->format->values_set);
    swfdec_text_field_movie_html_parse_close_tag (&data,
	(ParserTag *)data.tags_open->data, TRUE);
  }

  // add parsed styles
  while (data.tags_closed != NULL) {
    ParserTag *tag = (ParserTag *)data.tags_closed->data;

    if (tag->index != tag->end_index && tag->format != NULL) {
      swfdec_text_buffer_set_attributes (text->text, tag->index,
	  tag->end_index - tag->index, &tag->format->attr, tag->format->values_set);
    }

    g_free (tag);
    data.tags_closed = g_slist_remove (data.tags_closed, tag);
  }
}

/*
 * Generating
 */
static const char *
swfdec_text_field_movie_html_text_align_to_string (SwfdecTextAlign align)
{
  switch (align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      return "LEFT";
    case SWFDEC_TEXT_ALIGN_RIGHT:
      return "RIGHT";
    case SWFDEC_TEXT_ALIGN_CENTER:
      return "CENTER";
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      return "JUSTIFY";
    default:
      g_assert_not_reached ();
      return "";
  }
}

/*
 * Order of tags:
 * TEXTFORMAT / P or LI / FONT / A / B / I / U
 *
 * Order of attributes:
 * TEXTFORMAT:
 * LEFTMARGIN / RIGHTMARGIN / INDENT / LEADING / BLOCKINDENT / TABSTOPS
 * P: ALIGN
 * LI: none
 * FONT: FACE / SIZE / COLOR / LETTERSPACING / KERNING
 * A: HREF / TARGET
 * B: none
 * I: none
 * U: none
 */
static GString *
swfdec_text_field_movie_html_text_append_paragraph (SwfdecTextFieldMovie *text,
    GString *string, guint start_index, guint end_index)
{
  const SwfdecTextAttributes *attr, *attr_prev, *attr_font;
  SwfdecTextBufferIter *iter;
  GSList *fonts, *iter_font;
  guint index_, index_prev;
  gboolean textformat, bullet, font = FALSE;
  char *escaped;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), string);
  g_return_val_if_fail (string != NULL, string);
  g_return_val_if_fail (start_index <= end_index, string);

  iter = swfdec_text_buffer_get_iter (text->text, start_index);
  index_ = start_index;
  attr = swfdec_text_buffer_iter_get_attributes (text->text, iter);

  if (attr->left_margin != 0 || attr->right_margin != 0 ||
      attr->indent != 0 || attr->leading != 0 ||
      attr->block_indent != 0 ||
      attr->n_tab_stops > 0)
  {
    string = g_string_append (string, "<TEXTFORMAT");
    if (attr->left_margin) {
      g_string_append_printf (string, " LEFTMARGIN=\"%i\"",
	  attr->left_margin);
    }
    if (attr->right_margin) {
      g_string_append_printf (string, " RIGHTMARGIN=\"%i\"",
	  attr->right_margin);
    }
    if (attr->indent)
      g_string_append_printf (string, " INDENT=\"%i\"", attr->indent);
    if (attr->leading)
      g_string_append_printf (string, " LEADING=\"%i\"", attr->leading);
    if (attr->block_indent) {
      g_string_append_printf (string, " BLOCKINDENT=\"%i\"",
	  attr->block_indent);
    }
    if (attr->n_tab_stops > 0) {
      guint i;
      g_string_append (string, " TABSTOPS=\"\"");
      for (i = 0; i < attr->n_tab_stops; i++) {
	g_string_append_printf (string, "%d,", attr->tab_stops[i]);
      }
      string->str[string->len - 1] = '\"';
    }
    string = g_string_append (string, ">");

    textformat = TRUE;
  }
  else
  {
    textformat = FALSE;
  }

  if (attr->bullet) {
    string = g_string_append (string, "<LI>");
    bullet = TRUE;
  } else {
    g_string_append_printf (string, "<P ALIGN=\"%s\">",
	swfdec_text_field_movie_html_text_align_to_string (attr->align));
    bullet = FALSE;
  }

  // note we don't escape attr->font, even thought it can have evil chars
  g_string_append_printf (string, "<FONT FACE=\"%s\" SIZE=\"%i\" COLOR=\"#%06X\" LETTERSPACING=\"%i\" KERNING=\"%i\">",
      attr->font, attr->size, attr->color, (int)attr->letter_spacing,
      (attr->kerning ? 1 : 0));
  fonts = g_slist_prepend (NULL, (gpointer) attr);

  if (attr->url != SWFDEC_AS_STR_EMPTY)
    g_string_append_printf (string, "<A HREF=\"%s\" TARGET=\"%s\">",
	attr->url, attr->target);
  if (attr->bold)
    string = g_string_append (string, "<B>");
  if (attr->italic)
    string = g_string_append (string, "<I>");
  if (attr->underline)
    string = g_string_append (string, "<U>");

  // special case: use <= instead of < to add some extra markup
  for (iter = swfdec_text_buffer_iter_next (text->text, iter);
      iter != NULL && swfdec_text_buffer_iter_get_start (text->text, iter) <= end_index;
      iter = swfdec_text_buffer_iter_next (text->text, iter))
  {
    index_prev = index_;
    attr_prev = attr;
    index_ = swfdec_text_buffer_iter_get_start (text->text, iter);
    attr = swfdec_text_buffer_iter_get_attributes (text->text, iter);

    escaped = swfdec_xml_escape_len (swfdec_text_buffer_get_text (text->text) + index_prev,
	index_ - index_prev);
    string = g_string_append (string, escaped);
    g_free (escaped);
    escaped = NULL;

    // Figure out what tags need to be rewritten
    if (attr->font != attr_prev->font ||
	attr->size != attr_prev->size ||
	attr->color != attr_prev->color ||
	(int)attr->letter_spacing != (int)attr_prev->letter_spacing ||
	attr->kerning != attr_prev->kerning) {
      font = TRUE;
    } else if (attr->url == attr_prev->url &&
	attr->target == attr_prev->target &&
	attr->bold == attr_prev->bold &&
	attr->italic == attr_prev->italic &&
	attr->underline == attr_prev->underline) {
      continue;
    }

    // Close tags
    for (iter_font = fonts; iter_font != NULL; iter_font = iter_font->next)
    {
      attr_font = iter_font->data;
      if (attr->font == attr_font->font &&
	attr->size == attr_font->size &&
	attr->color == attr_font->color &&
	(int)attr->letter_spacing == (int)attr_font->letter_spacing &&
	attr->kerning == attr_font->kerning) {
	break;
      }
    }
    if (attr_prev->underline)
      string = g_string_append (string, "</U>");
    if (attr_prev->italic)
      string = g_string_append (string, "</I>");
    if (attr_prev->bold)
      string = g_string_append (string, "</B>");
    if (attr_prev->url != SWFDEC_AS_STR_EMPTY)
      string = g_string_append (string, "</A>");
    if (iter_font != NULL) {
      while (fonts != iter_font) {
	string = g_string_append (string, "</FONT>");
	fonts = g_slist_remove (fonts, fonts->data);
      }
    }

    // Open tags
    attr_font = fonts->data;
    if (font && (attr->font != attr_font->font ||
	 attr->size != attr_font->size ||
	 attr->color != attr_font->color ||
	 (int)attr->letter_spacing != (int)attr_font->letter_spacing ||
	 attr->kerning != attr_font->kerning))
    {
      fonts = g_slist_prepend (fonts, (gpointer) attr);

      string = g_string_append (string, "<FONT");
      // note we don't escape attr->font, even thought it can have evil chars
      if (attr->font != attr_font->font)
	g_string_append_printf (string, " FACE=\"%s\"", attr->font);
      if (attr->size != attr_font->size)
	g_string_append_printf (string, " SIZE=\"%i\"", attr->size);
      if (attr->color != attr_font->color)
	g_string_append_printf (string, " COLOR=\"#%06X\"", attr->color);
      if ((int)attr->letter_spacing != (int)attr_font->letter_spacing) {
	g_string_append_printf (string, " LETTERSPACING=\"%i\"",
	    (int)attr->letter_spacing);
      }
      if (attr->kerning != attr_font->kerning) {
	g_string_append_printf (string, " KERNING=\"%i\"",
	    (attr->kerning ? 1 : 0));
      }
      string = g_string_append (string, ">");
    }
    if (attr->url != SWFDEC_AS_STR_EMPTY) {
      g_string_append_printf (string, "<A HREF=\"%s\" TARGET=\"%s\">",
	  attr->url, attr->target);
    }
    if (attr->bold)
      string = g_string_append (string, "<B>");
    if (attr->italic)
      string = g_string_append (string, "<I>");
    if (attr->underline)
      string = g_string_append (string, "<U>");
  }

  escaped = swfdec_xml_escape_len (swfdec_text_buffer_get_text (text->text) + index_,
      end_index - index_);
  string = g_string_append (string, escaped);
  g_free (escaped);

  if (attr->underline)
    string = g_string_append (string, "</U>");
  if (attr->italic)
    string = g_string_append (string, "</I>");
  if (attr->bold)
    string = g_string_append (string, "</B>");
  if (attr->url != SWFDEC_AS_STR_EMPTY)
    string = g_string_append (string, "</A>");
  for (iter_font = fonts; iter_font != NULL; iter_font = iter_font->next)
    string = g_string_append (string, "</FONT>");
  g_slist_free (fonts);
  if (bullet) {
    string = g_string_append (string, "</LI>");
  } else {
    string = g_string_append (string, "</P>");
  }
  if (textformat)
    string = g_string_append (string, "</TEXTFORMAT>");

  return string;
}

const char *
swfdec_text_field_movie_get_html_text (SwfdecTextFieldMovie *text)
{
  const char *p, *end, *start;
  GString *string;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text),
      SWFDEC_AS_STR_EMPTY);

  string = g_string_new ("");
  p = start = swfdec_text_buffer_get_text (text->text);

  while (*p != '\0') {
    end = strpbrk (p, "\r\n");
    if (end == NULL)
      end = strchr (p, '\0');

    string = swfdec_text_field_movie_html_text_append_paragraph (text, string,
	p - start, end - start);

    p = end;
    if (*p != '\0') p++;
  }

  return swfdec_as_context_give_string (SWFDEC_AS_OBJECT (text)->context,
      g_string_free (string, FALSE));
}
