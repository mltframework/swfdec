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
#include "swfdec_xml.h"
#include "swfdec_debug.h"

typedef struct {
  const char *		name;
  int			name_length;
  guint			index;
  guint			end_index;
  SwfdecTextFormat	*format;
} ParserTag;

typedef struct {
  SwfdecAsContext	*cx;
  gboolean		multiline;
  GString *		text;
  GSList *		tags_open;
  GSList *		tags_closed;
} ParserData;

static void
swfdec_text_field_movie_html_parse_close_tag (ParserData *data, ParserTag *tag)
{
  if (data->multiline &&
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
	n->index = data->text->len;
	n->end_index = data->text->len + 1;
	n->format = swfdec_text_format_copy (f->format);
	data->tags_closed = g_slist_prepend (data->tags_closed, n);
	break;
      }
    }
    data->text = g_string_append_c (data->text, '\r');
  }

  tag->end_index = data->text->len;

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

  end = strstr (p, "-->");
  if (end != NULL)
    end += strlen("-->");

  // return NULL if no end found
  return end;
}

static void
swfdec_text_field_movie_html_tag_set_attribute (ParserTag *tag,
    const char *name, int name_length, const char *value, int value_length)
{
  SwfdecAsValue val;
  SwfdecAsObject *object;

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
}

static const char *
swfdec_text_field_movie_html_parse_attribute (ParserTag *tag, const char *p)
{
  const char *end, *name, *value;
  int name_length, value_length;

  g_return_val_if_fail ((*p != '>' && *p != '\0'), p);

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
    swfdec_text_field_movie_html_tag_set_attribute (tag, name, name_length,
	value, value_length);
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
    if (data->tags_open != NULL) {
      tag = data->tags_open->data;
      if (name_length == tag->name_length &&
	  !g_strncasecmp (name, tag->name, name_length))
	swfdec_text_field_movie_html_parse_close_tag (data, tag);
    }

    end = strchr (end, '>');
    if (end != NULL)
      end += 1;
  }
  else
  {
    if (data->cx->version < 7 &&
	(name_length == 2 && !g_strncasecmp (name, "br", 2))) {
      data->text = g_string_append_c (data->text, '\r');
      tag = NULL;
    } else {
      SwfdecAsObject *object;
      SwfdecAsValue val;

      if (data->cx->version < 7 &&
	  ((name_length == 1 && !g_strncasecmp (name, "p", 1)) ||
	   (name_length == 2 && !g_strncasecmp (name, "li", 2))))
      {
	GSList *iter;

	for (iter = data->tags_open; iter != NULL; iter = iter->next) {
	  ParserTag *f = iter->data;
	  if ((f->name_length == 1 && !g_strncasecmp (f->name, "p", 1)) ||
	      (f->name_length == 2 && !g_strncasecmp (f->name, "li", 2))) {
	    data->text = g_string_append_c (data->text, '\r');
	    break;
	  }
	}
      }

      tag = g_new0 (ParserTag, 1);
      tag->name = name;
      tag->name_length = name_length;
      tag->format = SWFDEC_TEXT_FORMAT (swfdec_text_format_new (data->cx));
      tag->index = data->text->len;

      data->tags_open = g_slist_prepend (data->tags_open, tag);

      // set format based on tag
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
    }

    // parse attributes
    end = end + strspn (end, " \r\n\t");
    while (*end != '\0' && *end != '>' && (*end != '/' || *(end + 1) != '>')) {
      end = swfdec_text_field_movie_html_parse_attribute (tag, end);
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
swfdec_text_field_movie_html_parse_text (ParserData *data, const char *p,
    gboolean condense_white)
{
  const char *end;
  char *unescaped;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (p != NULL, NULL);
  g_return_val_if_fail (*p != '\0' && *p != '<', NULL);

  // get the text
  // if condense_white: all whitespace blocks are converted to a single space
  // if not: all \n are converted to \r
  while (*p != '\0' && *p != '<') {
    if (condense_white) {
      end = p + strcspn (p, "< \n\r\t");
    } else {
      end = p + strcspn (p, "<\n");
    }

    unescaped = swfdec_xml_unescape_len (data->cx, p, end - p);
    data->text = g_string_append (data->text, unescaped);
    g_free (unescaped);

    if (g_ascii_isspace (*end)) {
      if (condense_white) {
	data->text = g_string_append_c (data->text, ' ');
	p = end + strspn (end, " \n\r\t");
      } else {
	data->text = g_string_append_c (data->text, '\r');
	p = end + 1;
      }
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
  data.multiline = (data.cx->version < 7 || text->text->multiline);
  data.text = g_string_new ("");
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
      p = swfdec_text_field_movie_html_parse_text (&data, p,
	  text->condense_white);
    }
  }

  // close remaining tags
  while (data.tags_open != NULL) {
    swfdec_text_field_movie_html_parse_close_tag (&data,
	(ParserTag *)data.tags_open->data);
  }

  // set parsed text
  text->text_display =
    swfdec_as_context_give_string (data.cx, g_string_free (data.text, FALSE));

  // add parsed styles
  while (data.tags_closed != NULL) {
    ParserTag *tag = (ParserTag *)data.tags_closed->data;

    if (tag->index != tag->end_index) {
      swfdec_text_field_movie_set_text_format (text, tag->format, tag->index,
	  tag->end_index);
    }

    data.tags_closed = g_slist_remove (data.tags_closed, tag);
  }
}
