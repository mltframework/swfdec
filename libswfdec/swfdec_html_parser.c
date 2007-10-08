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

  if (tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1)) {
    if (name_length == 5 && !g_strncasecmp (name, "align", 5)) {
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_align, &val);
    }
  }
}

static const char *
swfdec_edit_text_movie_html_parse_attribute (ParserTag *tag, const char *p)
{
  const char *end, *name, *value;
  int name_length, value_length;

  g_return_val_if_fail (tag != NULL, NULL);
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

  swfdec_edit_text_movie_html_tag_set_attribute (tag, name, name_length, value,
      value_length);

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
      // add paragraph for closing P and LI tags
      if ((name_length == 1 && !g_strncasecmp (name, "p", 1)) ||
	  (name_length == 2 && !g_strncasecmp (name, "li", 2))) {
	data->text = g_string_append_c (data->text, '\n');
      }

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
      tag = g_new0 (ParserTag, 1);
      tag->name = name;
      tag->name_length = name_length;
      tag->format = SWFDEC_TEXT_FORMAT (swfdec_text_format_new (data->cx));
      tag->index = data->text->len;
      data->tags_open = g_slist_prepend (data->tags_open, tag);
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

    // add paragraph for closing P and LI tags
    if ((tag->name_length == 1 && !g_strncasecmp (tag->name, "p", 1)) ||
	(tag->name_length == 2 && !g_strncasecmp (tag->name, "li", 2))) {
      data.text = g_string_append_c (data.text, '\n');
    }
    tag->end_index = data.text->len;

    data.tags_open = g_slist_remove (data.tags_open, tag);
    data.tags_closed = g_slist_prepend (data.tags_closed, tag);
  }

  // set parsed text
  text->text_display = swfdec_as_context_give_string (data.cx,
      g_string_free (data.text, FALSE));

  // add parsed styles
  while (data.tags_closed != NULL) {
    ParserTag *tag = (ParserTag *)data.tags_closed->data;

    swfdec_edit_text_movie_set_text_format (text, tag->format, tag->index,
	tag->end_index);

    data.tags_closed = g_slist_remove (data.tags_closed, tag);
  }
}
