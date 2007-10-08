#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "swfdec_edittext_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_font.h"
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
  GString *		text;
  GSList *		tags_open;
  GSList *		tags_closed;
} ParserData;

static const char *
swfdec_edit_text_movie_html_parse_comment (ParserData *data, const char *p)
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
swfdec_edit_text_movie_html_tag_set_attribute (ParserTag *tag,
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
    else if (name_length == 13 && !g_strncasecmp (name, "letterspacing", 13))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_letterSpacing,
	  &val);
    }
    else if (name_length == 7 && !g_strncasecmp (name, "kerning", 7))
    {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_kerning, &val);
    }
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
swfdec_edit_text_movie_html_parse_attribute (ParserTag *tag, const char *p)
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
    swfdec_edit_text_movie_html_tag_set_attribute (tag, name, name_length,
	value, value_length);
  }

  g_return_val_if_fail (end + 1 > p, NULL);

  return end + 1;
}

static const char *
swfdec_edit_text_movie_html_parse_tag (ParserData *data, const char *p)
{
  ParserTag *tag;
  const char *name, *end;
  int name_length;
  gboolean close;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (p != NULL, NULL);
  g_return_val_if_fail (*p == '<', NULL);

  // closing tag or opening tag?
  if (*(p + 1) == '/') {
    close = TRUE;
    p++;
  } else {
    close = FALSE;
  }

  // find the end of the name
  end = p + strcspn (p, "> \r\n\t");
  if (*end == '\0')
    return end;

  // don't count trailing / as part of the name if it's followed by >
  if (*end == '>' && *(end - 1) == '/')
    end = end - 1;

  if (end == p + 1) // empty name
    return NULL;

  name = p + 1;
  name_length = end - (p + 1);

  if (close)
  {
    if (data->tags_open != NULL) {
      tag = (ParserTag *)data->tags_open->data;
    } else {
      tag = NULL;
    }

    if (tag != NULL && name_length == tag->name_length &&
	!g_strncasecmp (name, tag->name, name_length))
    {
      tag->end_index = data->text->len;

      data->tags_open = g_slist_remove (data->tags_open, tag);
      data->tags_closed = g_slist_prepend (data->tags_closed, tag);
    }

    end = strchr (end, '>');
    if (end != NULL)
      end += 1;
  }
  else
  {
    if (name_length == 2 && !g_strncasecmp (name, "br", 2)) {
      tag = NULL;
      data->text = g_string_append_c (data->text, '\n');
    } else {
      SwfdecAsObject *object;
      SwfdecAsValue val;

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
      end = swfdec_edit_text_movie_html_parse_attribute (tag, end);
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
swfdec_edit_text_movie_html_parse_text (ParserData *data, const char *p)
{
  const char *end;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (p != NULL, NULL);
  g_return_val_if_fail (*p != '\0' && *p != '<', NULL);

  end = strchr (p, '<');
  if (end == NULL)
    end = strchr (p, '\0');

  data->text = g_string_append_len (data->text, p, end - p);

  return end;
}

void
swfdec_edit_text_movie_html_parse (SwfdecEditTextMovie *text, const char *str)
{
  ParserData data;
  const char *p;
  char *string, *r;

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT_MOVIE (text));
  g_return_if_fail (str != NULL);

  data.cx = SWFDEC_AS_OBJECT (text)->context;
  data.text = g_string_new ("");
  data.tags_open = NULL;
  data.tags_closed = NULL;

  p = str;
  while (*p != '\0' && p != NULL) {
    if (*p == '<') {
      if (strncmp (p + 1, "!--", strlen ("!--")) == 0) {
	p = swfdec_edit_text_movie_html_parse_comment (&data, p);
      } else {
	p = swfdec_edit_text_movie_html_parse_tag (&data, p);
      }
    } else {
      p = swfdec_edit_text_movie_html_parse_text (&data, p);
    }
  }

  // close remaining tags
  while (data.tags_open != NULL) {
    ParserTag *tag = (ParserTag *)data.tags_open->data;
    tag->end_index = data.text->len;

    data.tags_open = g_slist_remove (data.tags_open, tag);
    data.tags_closed = g_slist_prepend (data.tags_closed, tag);
  }

  // set parsed text
  // change all \n to \r
  string = g_string_free (data.text, FALSE);
  r = string;
  while ((r = strchr (r, '\n')) != NULL) {
    *r = '\r';
  }
  text->text_display = swfdec_as_context_give_string (data.cx, string);

  // add parsed styles
  while (data.tags_closed != NULL) {
    ParserTag *tag = (ParserTag *)data.tags_closed->data;

    swfdec_edit_text_movie_set_text_format (text, tag->format, tag->index,
	tag->end_index);

    data.tags_closed = g_slist_remove (data.tags_closed, tag);
  }
}
