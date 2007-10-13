/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "swfdec_text_field.h"
#include "swfdec_text_field_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_xml.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"

static void
swfdec_text_field_movie_get_text (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_STRING (ret, (text->text_display != NULL ?
	swfdec_as_context_get_string (cx, text->text_display) :
	SWFDEC_AS_STR_EMPTY));
}

static void
swfdec_text_field_movie_do_set_text (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "s", &value);

  swfdec_text_field_movie_set_text (text, value, FALSE);
}

static const char *
align_to_string (SwfdecTextAlign align)
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
swfdec_text_field_movie_htmlText_append_paragraph (SwfdecTextFieldMovie *text,
    GString *string, guint start_index, guint end_index)
{
  SwfdecTextFormat *format, *format_prev, *format_font;
  GSList *iter, *fonts, *iter_font;
  guint index_, index_prev;
  gboolean textformat, bullet, font;
  char *escaped;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), string);
  g_return_val_if_fail (string != NULL, string);
  g_return_val_if_fail (start_index < end_index, string);

  g_return_val_if_fail (text->formats != NULL, string);
  for (iter = text->formats; iter->next != NULL &&
      ((SwfdecFormatIndex *)(iter->next->data))->index <= start_index;
      iter = iter->next);

  index_ = start_index;
  format = ((SwfdecFormatIndex *)(iter->data))->format;

  if (format->left_margin != 0 || format->right_margin != 0 ||
      format->indent != 0 || format->leading != 0 ||
      format->block_indent != 0 ||
      swfdec_as_array_get_length (format->tab_stops) > 0)
  {
    string = g_string_append (string, "<TEXTFORMAT");
    if (format->left_margin) {
      g_string_append_printf (string, " LEFTMARGIN=\"%i\"",
	  format->left_margin);
    }
    if (format->right_margin) {
      g_string_append_printf (string, " RIGHTMARGIN=\"%i\"",
	  format->right_margin);
    }
    if (format->indent)
      g_string_append_printf (string, " INDENT=\"%i\"", format->indent);
    if (format->leading)
      g_string_append_printf (string, " LEADING=\"%i\"", format->leading);
    if (format->block_indent) {
      g_string_append_printf (string, " BLOCKINDENT=\"%i\"",
	  format->block_indent);
    }
    if (swfdec_as_array_get_length (format->tab_stops) > 0) {
      SwfdecAsValue val;
      SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT  (format->tab_stops));
      g_string_append_printf (string, " TABSTOPS=\"%s\"",
	  swfdec_as_value_to_string (SWFDEC_AS_OBJECT
	    (format->tab_stops)->context, &val));
    }
    string = g_string_append (string, ">");

    textformat = TRUE;
  }
  else
  {
    textformat = FALSE;
  }

  if (format->bullet) {
    string = g_string_append (string, "<LI>");
    bullet = TRUE;
  } else {
    g_string_append_printf (string, "<P ALIGN=\"%s\">",
	align_to_string (format->align));
    bullet = FALSE;
  }

  // note we don't escape format->font, even thought it can have evil chars
  g_string_append_printf (string, "<FONT FACE=\"%s\" SIZE=\"%i\" COLOR=\"#%06X\" LETTERSPACING=\"%i\" KERNING=\"%i\">",
      format->font, format->size, format->color, (int)format->letter_spacing,
      (format->kerning ? 1 : 0));
  fonts = g_slist_prepend (NULL, format);

  if (format->url != SWFDEC_AS_STR_EMPTY)
    g_string_append_printf (string, "<A HREF=\"%s\" TARGET=\"%s\">",
	format->url, format->target);
  if (format->bold)
    string = g_string_append (string, "<B>");
  if (format->italic)
    string = g_string_append (string, "<I>");
  if (format->underline)
    string = g_string_append (string, "<U>");

  // special case: use <= instead of < to add some extra markup
  for (iter = iter->next;
      iter != NULL && ((SwfdecFormatIndex *)(iter->data))->index <= end_index;
      iter = iter->next)
  {
    index_prev = index_;
    format_prev = format;
    index_ = ((SwfdecFormatIndex *)(iter->data))->index;
    format = ((SwfdecFormatIndex *)(iter->data))->format;

    escaped = swfdec_xml_escape_len (text->text_display + index_prev,
	index_ - index_prev);
    string = g_string_append (string, escaped);
    g_free (escaped);
    escaped = NULL;

    // Figure out what tags need to be rewritten
    if (format->font != format_prev->font ||
	format->size != format_prev->size ||
	format->color != format_prev->color ||
	(int)format->letter_spacing != (int)format_prev->letter_spacing ||
	format->kerning != format_prev->kerning) {
      font = TRUE;
    } else if (format->url == format_prev->url &&
	format->target == format_prev->target &&
	format->bold == format_prev->bold &&
	format->italic == format_prev->italic &&
	format->underline == format_prev->underline) {
      continue;
    }

    // Close tags
    for (iter_font = fonts; iter_font != NULL; iter_font = iter_font->next)
    {
      format_font = (SwfdecTextFormat *)iter_font->data;
      if (format->font == format_font->font &&
	format->size == format_font->size &&
	format->color == format_font->color &&
	(int)format->letter_spacing == (int)format_font->letter_spacing &&
	format->kerning == format_font->kerning) {
	break;
      }
    }
    if (iter_font != NULL) {
      while (fonts != iter_font) {
	string = g_string_append (string, "</FONT>");
	fonts = g_slist_remove (fonts, fonts->data);
      }
    }
    if (format_prev->underline)
      string = g_string_append (string, "</U>");
    if (format_prev->italic)
      string = g_string_append (string, "</I>");
    if (format_prev->bold)
      string = g_string_append (string, "</B>");
    if (format_prev->url != SWFDEC_AS_STR_EMPTY)
      string = g_string_append (string, "</A>");

    // Open tags
    format_font = (SwfdecTextFormat *)fonts->data;
    if (font && (format->font != format_font->font ||
	 format->size != format_font->size ||
	 format->color != format_font->color ||
	 (int)format->letter_spacing != (int)format_font->letter_spacing ||
	 format->kerning != format_font->kerning))
    {
      fonts = g_slist_prepend (fonts, format);

      string = g_string_append (string, "<FONT");
      // note we don't escape format->font, even thought it can have evil chars
      if (format->font != format_font->font)
	g_string_append_printf (string, " FACE=\"%s\"", format->font);
      if (format->size != format_font->size)
	g_string_append_printf (string, " SIZE=\"%i\"", format->size);
      if (format->color != format_font->color)
	g_string_append_printf (string, " COLOR=\"#%06X\"", format->color);
      if ((int)format->letter_spacing != (int)format_font->letter_spacing) {
	g_string_append_printf (string, " LETTERSPACING=\"%i\"",
	    (int)format->letter_spacing);
      }
      if (format->kerning != format_font->kerning) {
	g_string_append_printf (string, " KERNING=\"%i\"",
	    (format->kerning ? 1 : 0));
      }
      string = g_string_append (string, ">");
    }
    if (format->url != SWFDEC_AS_STR_EMPTY) {
      g_string_append_printf (string, "<A HREF=\"%s\" TARGET=\"%s\">",
	  format->url, format->target);
    }
    if (format->bold)
      string = g_string_append (string, "<B>");
    if (format->italic)
      string = g_string_append (string, "<I>");
    if (format->underline)
      string = g_string_append (string, "<U>");
  }

  escaped = swfdec_xml_escape_len (text->text_display + index_,
      end_index - index_);
  string = g_string_append (string, escaped);
  g_free (escaped);

  if (format->underline)
    string = g_string_append (string, "</U>");
  if (format->italic)
    string = g_string_append (string, "</I>");
  if (format->bold)
    string = g_string_append (string, "</B>");
  if (format->url != SWFDEC_AS_STR_EMPTY)
    string = g_string_append (string, "</A>");
  for (iter = fonts; iter != NULL; iter = iter->next)
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

static void
swfdec_text_field_movie_get_htmlText (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  const char *p, *end;
  GString *string;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (text->text->html == FALSE) {
    swfdec_text_field_movie_get_text (cx, object, argc, argv, ret);
    return;
  }

  if (text->text_display == NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }

  string = g_string_new ("");

  p = text->text_display;
  while (*p != '\0') {
    end = strchr (p, '\r');
    if (end == NULL)
      end = strchr (p, '\0');

    string = swfdec_text_field_movie_htmlText_append_paragraph (text, string,
	p - text->text_display, end - text->text_display);

    if (*end == '\r') {
      p = end + 1;
    } else {
      p = end;
    }
  }

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_as_context_give_string (cx, g_string_free (string, FALSE)));
}

static void
swfdec_text_field_movie_set_htmlText (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "s", &value);

  swfdec_text_field_movie_set_text (text, value, text->text->html);
}

static void
swfdec_text_field_movie_get_html (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->html);
}

static void
swfdec_text_field_movie_set_html (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  text->text->html = value;
}

static void
swfdec_text_field_movie_get_condenseWhite (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->condense_white);
}

static void
swfdec_text_field_movie_set_condenseWhite (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  text->condense_white = value;
}

static void
swfdec_text_field_movie_get_embedFonts (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->embed_fonts);
}

static void
swfdec_text_field_movie_set_embedFonts (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  text->text->embed_fonts = value;

  if (text->text->embed_fonts)
    SWFDEC_FIXME ("Using embed fonts in TextField not supported");
}

// does nothing but calls valueOf
static void
swfdec_text_field_movie_set_length (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  swfdec_as_value_to_number (cx, &argv[0]);
}

static void
swfdec_text_field_movie_get_length (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_INT (ret, strlen (text->text_display));
}

static void
swfdec_text_field_movie_get_textColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (text->format_new),
      SWFDEC_AS_STR_color, ret);
}

// This doesn't work the same way as TextFormat's color setting
static void
swfdec_text_field_movie_set_textColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  int value;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "i", &value);

  if (value < 0) {
    SWFDEC_AS_VALUE_SET_NUMBER (&val, 16777216 + value % 16777216);
  } else {
    SWFDEC_AS_VALUE_SET_NUMBER (&val, value);
  }

  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (text->format_new),
      SWFDEC_AS_STR_color, &val);
}

static void
swfdec_text_field_movie_get_autoSize (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  switch (text->text->auto_size) {
    case SWFDEC_AUTO_SIZE_NONE:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_none);
      break;
    case SWFDEC_AUTO_SIZE_LEFT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_left);
      break;
    case SWFDEC_AUTO_SIZE_RIGHT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_right);
      break;
    case SWFDEC_AUTO_SIZE_CENTER:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_center);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_field_movie_set_autoSize (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  SwfdecAutoSize old;
  const char *s;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_BOOLEAN (&argv[0])) {
    if (SWFDEC_AS_VALUE_GET_BOOLEAN (&argv[0])) {
      text->text->auto_size = SWFDEC_AUTO_SIZE_LEFT;
    } else {
      text->text->auto_size = SWFDEC_AUTO_SIZE_NONE;
    }
    return;
  }

  swfdec_as_value_to_number (cx, &argv[0]);
  s = swfdec_as_value_to_string (cx, &argv[0]);

  old = text->text->auto_size;
  if (!g_ascii_strcasecmp (s, "none")) {
    text->text->auto_size = SWFDEC_AUTO_SIZE_NONE;
  } else if (!g_ascii_strcasecmp (s, "left")) {
    text->text->auto_size = SWFDEC_AUTO_SIZE_LEFT;
  } else if (!g_ascii_strcasecmp (s, "right")) {
    text->text->auto_size = SWFDEC_AUTO_SIZE_RIGHT;
  } else if (!g_ascii_strcasecmp (s, "center")) {
    text->text->auto_size = SWFDEC_AUTO_SIZE_CENTER;
  }

  if (text->text->auto_size != old)
    swfdec_text_field_movie_format_changed (text);
}

static void
swfdec_text_field_movie_get_wordWrap (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->word_wrap);
}

static void
swfdec_text_field_movie_set_wordWrap (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  if (text->text->word_wrap != value) {
    text->text->word_wrap = value;
    swfdec_text_field_movie_format_changed (text);
  }
}

static void
swfdec_text_field_movie_get_border (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->border);
}

static void
swfdec_text_field_movie_set_border (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  if (text->text->border != value) {
    text->text->border = value;
    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
  }
}

SWFDEC_AS_NATIVE (104, 104, swfdec_text_field_movie_getNewTextFormat)
void
swfdec_text_field_movie_getNewTextFormat (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_OBJECT (ret,
      SWFDEC_AS_OBJECT (swfdec_text_format_copy (text->format_new)));
}

SWFDEC_AS_NATIVE (104, 105, swfdec_text_field_movie_setNewTextFormat)
void
swfdec_text_field_movie_setNewTextFormat (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  SwfdecAsObject *obj;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "o", &obj);

  if (!SWFDEC_IS_TEXT_FORMAT (obj))
    return;

  swfdec_text_format_add (text->format_new, SWFDEC_TEXT_FORMAT (obj));
}

SWFDEC_AS_NATIVE (104, 102, swfdec_text_field_movie_setTextFormat)
void
swfdec_text_field_movie_setTextFormat (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  SwfdecTextFormat *format;
  guint start_index, end_index;
  int i;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (argc < 1)
    return;

  i = 0;
  if (argc >= 2) {
    start_index = swfdec_as_value_to_integer (cx, &argv[i++]);
    start_index = MIN (start_index, strlen (text->text_display));
  } else {
    start_index = 0;
  }
  if (argc >= 3) {
    end_index = swfdec_as_value_to_integer (cx, &argv[i++]);
    end_index = CLAMP (end_index, start_index, strlen (text->text_display));
  } else {
    end_index = strlen (text->text_display);
  }
  if (start_index == end_index)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[i]))
    return;
  if (!SWFDEC_IS_TEXT_FORMAT (SWFDEC_AS_VALUE_GET_OBJECT (&argv[i])))
    return;

  format = SWFDEC_TEXT_FORMAT (SWFDEC_AS_VALUE_GET_OBJECT (&argv[i]));

  swfdec_text_field_movie_set_text_format (text, format, start_index, end_index);
}

SWFDEC_AS_NATIVE (104, 200, swfdec_text_field_movie_createTextField)
void
swfdec_text_field_movie_createTextField (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SwfdecMovie *movie, *parent;
  SwfdecTextField *edittext;
  int depth, x, y, width, height;
  const char *name;
  SwfdecAsFunction *fun;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &parent, "siiiii", &name, &depth, &x, &y, &width, &height);

  edittext = g_object_new (SWFDEC_TYPE_TEXT_FIELD, NULL);
  edittext->html = FALSE;
  edittext->input = FALSE;
  edittext->password = FALSE;
  edittext->selectable = TRUE;
  edittext->font = NULL; // FIXME
  edittext->word_wrap = FALSE;
  edittext->multiline = FALSE;
  edittext->auto_size = SWFDEC_AUTO_SIZE_NONE;
  edittext->border = FALSE;
  edittext->size = 240; // FIXME: Correct?

  edittext->text_input = NULL;
  edittext->variable = NULL;
  edittext->color = 0;
  edittext->align = SWFDEC_TEXT_ALIGN_LEFT;
  edittext->left_margin = 0;
  edittext->right_margin = 0;
  edittext->indent = 0;
  edittext->leading = 0;

  edittext->graphic.extents.x0 = SWFDEC_DOUBLE_TO_TWIPS (x);
  edittext->graphic.extents.x1 =
    edittext->graphic.extents.x0 + SWFDEC_DOUBLE_TO_TWIPS (width);
  edittext->graphic.extents.y0 = SWFDEC_DOUBLE_TO_TWIPS (y);
  edittext->graphic.extents.y1 =
    edittext->graphic.extents.y0 + SWFDEC_DOUBLE_TO_TWIPS (height);

  movie = swfdec_movie_find (parent, depth);
  if (movie)
    swfdec_movie_remove (movie);

  movie = swfdec_movie_new (SWFDEC_PLAYER (cx), depth, parent,
      SWFDEC_GRAPHIC (edittext), name);
  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (movie));
  swfdec_movie_initialize (movie);

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (&val);
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    return;

  /* set initial variables */
  if (swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (fun),
	SWFDEC_AS_STR_prototype, &val)) {
    swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_STR___proto__, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (fun));
  if (cx->version < 7) {
    swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_STR_constructor, &val, SWFDEC_AS_VARIABLE_HIDDEN);
  }
  swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (movie),
      SWFDEC_AS_STR___constructor__, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_VERSION_6_UP);

  swfdec_as_function_call (fun, SWFDEC_AS_OBJECT (movie), 0, NULL, rval);
  cx->frame->construct = TRUE;
  swfdec_as_context_run (cx);
}

static void
swfdec_text_field_movie_add_variable (SwfdecAsObject *object,
    const char *variable, SwfdecAsNative get, SwfdecAsNative set)
{
  SwfdecAsFunction *get_func, *set_func;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (get != NULL);

  get_func =
    swfdec_as_native_function_new (object->context, variable, get, 0, NULL);
  if (get_func == NULL)
    return;

  if (set != NULL) {
    set_func =
      swfdec_as_native_function_new (object->context, variable, set, 0, NULL);
  } else {
    set_func = NULL;
  }

  swfdec_as_object_add_variable (object, variable, get_func, set_func, 0);
}

void
swfdec_text_field_movie_init_properties (SwfdecAsContext *cx)
{
  SwfdecAsValue val;
  SwfdecAsObject *object, *proto;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  object = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  swfdec_as_object_get_variable (object, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_text,
      swfdec_text_field_movie_get_text, swfdec_text_field_movie_do_set_text);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_htmlText,
      swfdec_text_field_movie_get_htmlText,
      swfdec_text_field_movie_set_htmlText);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_html,
      swfdec_text_field_movie_get_html, swfdec_text_field_movie_set_html);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_condenseWhite,
      swfdec_text_field_movie_get_condenseWhite,
      swfdec_text_field_movie_set_condenseWhite);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_embedFonts,
      swfdec_text_field_movie_get_embedFonts,
      swfdec_text_field_movie_set_embedFonts);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_length,
      swfdec_text_field_movie_get_length,
      swfdec_text_field_movie_set_length);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_textColor,
      swfdec_text_field_movie_get_textColor,
      swfdec_text_field_movie_set_textColor);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_autoSize,
      swfdec_text_field_movie_get_autoSize,
      swfdec_text_field_movie_set_autoSize);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_wordWrap,
      swfdec_text_field_movie_get_wordWrap,
      swfdec_text_field_movie_set_wordWrap);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_border,
      swfdec_text_field_movie_get_border, swfdec_text_field_movie_set_border);
}

SWFDEC_AS_CONSTRUCTOR (104, 0, swfdec_text_field_movie_construct, swfdec_text_field_movie_get_type)
void
swfdec_text_field_movie_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!cx->frame->construct) {
    SwfdecAsValue val;
    if (!swfdec_as_context_use_mem (cx, sizeof (SwfdecTextFieldMovie)))
      return;
    object = g_object_new (SWFDEC_TYPE_TEXT_FIELD_MOVIE, NULL);
    swfdec_as_object_add (object, cx, sizeof (SwfdecTextFieldMovie));
    swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_set_constructor (object,
	  SWFDEC_AS_VALUE_GET_OBJECT (&val));
    } else {
      SWFDEC_INFO ("\"TextField\" is not an object");
    }
  }

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (object));

  swfdec_text_field_movie_init_properties (cx);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}
