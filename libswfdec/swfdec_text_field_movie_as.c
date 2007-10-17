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
#include "swfdec_as_object.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"

static SwfdecColor
swfdec_text_field_movie_int_to_color (SwfdecAsContext *cx, int value)
{
  if (value < 0) {
    return 16777216 + value % 16777216;
  } else {
    return value % 16777216;
  }
}

// does nothing but calls valueOf
static void
swfdec_text_field_movie_set_readonly (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (argc > 0)
    swfdec_as_value_to_number (cx, &argv[0]);
}

/*
 * Native properties: Text
 */
static void
swfdec_text_field_movie_get_text (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_STRING (ret, (text->text_display != NULL ?
	text->text_display : SWFDEC_AS_STR_EMPTY));
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

  if (text->variable != NULL) {
    swfdec_text_field_movie_set_listen_variable_text (text,
	swfdec_text_field_movie_get_html_text (text));
  }
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
swfdec_text_field_movie_get_htmlText (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_text_field_movie_get_html_text (text));
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

  if (text->variable != NULL) {
    swfdec_text_field_movie_set_listen_variable_text (text,
	swfdec_text_field_movie_get_html_text (text));
  }
}

static void
swfdec_text_field_movie_get_length (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_INT (ret, g_utf8_strlen (text->text_display, -1));
}

/*
 * Native properties: Input
 */
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
swfdec_text_field_movie_get_maxChars (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (text->text->max_chars != 0) {
    SWFDEC_AS_VALUE_SET_INT (ret, text->text->max_chars);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_text_field_movie_set_maxChars (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (argc < 1)
    return;

  swfdec_as_value_to_number (cx, &argv[0]);
  text->text->max_chars = swfdec_as_value_to_integer (cx, &argv[0]);
}

static void
swfdec_text_field_movie_get_multiline (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->multiline);
}

static void
swfdec_text_field_movie_set_multiline (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  text->text->multiline = value;
}

static void
swfdec_text_field_movie_get_restrict (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (text->restrict_ != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, text->restrict_);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_text_field_movie_set_restrict (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  const char *value;

  if (argc > 0)
    swfdec_as_value_to_number (cx, &argv[0]);

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "s", &value);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    text->restrict_ = NULL;
  } else {
    text->restrict_ = value;
  }
}

static void
swfdec_text_field_movie_get_selectable (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->selectable);
}

static void
swfdec_text_field_movie_set_selectable (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  text->text->selectable = value;

  // FIXME: If selection made and removing selectable force redraw?
}

static void
swfdec_text_field_movie_do_get_type (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (text->text->input) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_input);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_dynamic);
  }
}

static void
swfdec_text_field_movie_do_set_type (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  const char *value;

  if (argc > 0)
    swfdec_as_value_to_number (cx, &argv[0]);

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "s", &value);

  if (!g_strcasecmp (value, SWFDEC_AS_STR_input)) {
    text->text->input = TRUE;
  } else if (!g_strcasecmp (value, SWFDEC_AS_STR_dynamic)) {
    text->text->input = FALSE;
  }
}

static void
swfdec_text_field_movie_do_get_variable (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  if (text->variable) {
    SWFDEC_AS_VALUE_SET_STRING (ret, text->variable);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_text_field_movie_do_set_variable (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  const char *value;

  if (argc > 0)
    swfdec_as_value_to_number (cx, &argv[0]);

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "s", &value);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]) || value == SWFDEC_AS_STR_EMPTY) {
    value = NULL;
  }

  swfdec_text_field_movie_set_listen_variable (text, value);
}

/*
 * Native properties: Background & border
 */
static void
swfdec_text_field_movie_get_background (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->background);
}

static void
swfdec_text_field_movie_set_background (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  if (text->text->background != value) {
    text->text->background = value;
    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
  }
}

static void
swfdec_text_field_movie_get_backgroundColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_NUMBER (ret, text->background_color);
}

static void
swfdec_text_field_movie_set_backgroundColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  int value;
  SwfdecColor color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "i", &value);

  color = swfdec_text_field_movie_int_to_color (cx, value);
  if (text->background_color != color) {
    text->background_color = color;
    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
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

static void
swfdec_text_field_movie_get_borderColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_NUMBER (ret, text->border_color);
}

static void
swfdec_text_field_movie_set_borderColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  int value;
  SwfdecColor color;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "i", &value);

  color = swfdec_text_field_movie_int_to_color (cx, value);
  if (text->border_color != color) {
    text->border_color = color;
    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
  }
}

/*
 * Native properties: Scrolling
 */
static void
swfdec_text_field_movie_get_mouseWheelEnabled (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->mouse_wheel_enabled);
}

static void
swfdec_text_field_movie_set_mouseWheelEnabled (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  text->mouse_wheel_enabled = value;
}

static void
swfdec_text_field_movie_do_get_scroll (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_NUMBER (ret, text->text->scroll);
}

static void
swfdec_text_field_movie_do_set_scroll (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  int value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "i", &value);

  swfdec_text_field_movie_set_scroll (text, value);
}

/*
 * Native properties: Display
 */
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
swfdec_text_field_movie_get_password (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, text->text->password);
}

static void
swfdec_text_field_movie_set_password (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  gboolean value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "b", &value);

  swfdec_as_value_to_number (cx, &argv[0]);

  if (text->text->password != value) {
    text->text->password = value;
    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
  }
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

/*
 * Native properties: Format
 */
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

  if (!text->text->embed_fonts && value)
    SWFDEC_FIXME ("Using embed fonts in TextField not supported");

  text->text->embed_fonts = value;
}

static void
swfdec_text_field_movie_get_textColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "");

  SWFDEC_AS_VALUE_SET_NUMBER (ret, text->format_new->color);
}

// This doesn't work the same way as TextFormat's color setting
static void
swfdec_text_field_movie_set_textColor (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFieldMovie *text;
  int value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FIELD_MOVIE, &text, "i", &value);

  text->format_new->color = swfdec_text_field_movie_int_to_color (cx, value);
}

/*
 * Native functions
 */
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

/*
 * Creating TextFields
 */
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

  SWFDEC_GRAPHIC (edittext)->extents.x0 = SWFDEC_DOUBLE_TO_TWIPS (x);
  SWFDEC_GRAPHIC (edittext)->extents.x1 =
    SWFDEC_GRAPHIC (edittext)->extents.x0 + SWFDEC_DOUBLE_TO_TWIPS (width);
  SWFDEC_GRAPHIC (edittext)->extents.y0 = SWFDEC_DOUBLE_TO_TWIPS (y);
  SWFDEC_GRAPHIC (edittext)->extents.y1 =
    SWFDEC_GRAPHIC (edittext)->extents.y0 + SWFDEC_DOUBLE_TO_TWIPS (height);

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

  // text
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_text,
      swfdec_text_field_movie_get_text, swfdec_text_field_movie_do_set_text);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_html,
      swfdec_text_field_movie_get_html, swfdec_text_field_movie_set_html);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_htmlText,
      swfdec_text_field_movie_get_htmlText,
      swfdec_text_field_movie_set_htmlText);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_length,
      swfdec_text_field_movie_get_length,
      swfdec_text_field_movie_set_readonly);

  // input
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_condenseWhite,
      swfdec_text_field_movie_get_condenseWhite,
      swfdec_text_field_movie_set_condenseWhite);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_maxChars,
      swfdec_text_field_movie_get_maxChars,
      swfdec_text_field_movie_set_maxChars);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_multiline,
      swfdec_text_field_movie_get_multiline,
      swfdec_text_field_movie_set_multiline);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_restrict,
      swfdec_text_field_movie_get_restrict,
      swfdec_text_field_movie_set_restrict);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_selectable,
      swfdec_text_field_movie_get_selectable,
      swfdec_text_field_movie_set_selectable);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_type,
      swfdec_text_field_movie_do_get_type,
      swfdec_text_field_movie_do_set_type);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_variable,
      swfdec_text_field_movie_do_get_variable,
      swfdec_text_field_movie_do_set_variable);

  // info
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_textHeight,
      swfdec_text_field_movie_get_textHeight,
      swfdec_text_field_movie_set_readonly);*/
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_textWidth,
      swfdec_text_field_movie_get_textWidth,
      swfdec_text_field_movie_set_readonly);*/

  // border & background
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_background,
      swfdec_text_field_movie_get_background,
      swfdec_text_field_movie_set_background);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_backgroundColor,
      swfdec_text_field_movie_get_backgroundColor,
      swfdec_text_field_movie_set_backgroundColor);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_border,
      swfdec_text_field_movie_get_border, swfdec_text_field_movie_set_border);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_borderColor,
      swfdec_text_field_movie_get_borderColor,
      swfdec_text_field_movie_set_borderColor);

  // scrolling
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_bottomScroll,
      swfdec_text_field_movie_get_bottomScroll,
      swfdec_text_field_movie_set_readonly);*/
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_hscroll,
      swfdec_text_field_movie_get_hscroll,
      swfdec_text_field_movie_set_hscroll);*/
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_maxhscroll,
      swfdec_text_field_movie_get_maxhscroll,
      swfdec_text_field_movie_set_readonly);*/
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_maxscroll,
      swfdec_text_field_movie_get_maxscroll,
      swfdec_text_field_movie_set_readonly);*/
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_mouseWheelEnabled,
      swfdec_text_field_movie_get_mouseWheelEnabled,
      swfdec_text_field_movie_set_mouseWheelEnabled);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_scroll,
      swfdec_text_field_movie_do_get_scroll,
      swfdec_text_field_movie_do_set_scroll);

  // display
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_autoSize,
      swfdec_text_field_movie_get_autoSize,
      swfdec_text_field_movie_set_autoSize);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_password,
      swfdec_text_field_movie_get_password,
      swfdec_text_field_movie_set_password);
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_wordWrap,
      swfdec_text_field_movie_get_wordWrap,
      swfdec_text_field_movie_set_wordWrap);

  // format
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_antiAliasType,
      swfdec_text_field_movie_get_antiAliasType,
      swfdec_text_field_movie_set_antiAliasType);*/
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_embedFonts,
      swfdec_text_field_movie_get_embedFonts,
      swfdec_text_field_movie_set_embedFonts);
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_gridFitType,
      swfdec_text_field_movie_get_gridFitType,
      swfdec_text_field_movie_set_gridFitType);*/
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_sharpness,
      swfdec_text_field_movie_get_sharpness,
      swfdec_text_field_movie_set_sharpness);*/
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_styleSheet,
      swfdec_text_field_movie_get_styleSheet,
      swfdec_text_field_movie_set_styleSheet);*/
  swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_textColor,
      swfdec_text_field_movie_get_textColor,
      swfdec_text_field_movie_set_textColor);
  /*swfdec_text_field_movie_add_variable (proto, SWFDEC_AS_STR_thickness,
      swfdec_text_field_movie_get_thickness,
      swfdec_text_field_movie_set_thickness);*/

  // TODO: filters, menu, tabEnabled, tabIndex
}

SWFDEC_AS_NATIVE (104, 0, swfdec_text_field_movie_construct)
void
swfdec_text_field_movie_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!cx->frame->construct) {
    SwfdecAsValue val;
    if (!swfdec_as_context_use_mem (cx, sizeof (SwfdecAsObject)))
      return;
    object = g_object_new (SWFDEC_TYPE_AS_OBJECT, NULL);
    swfdec_as_object_add (object, cx, sizeof (SwfdecAsObject));
    swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_set_constructor (object,
	  SWFDEC_AS_VALUE_GET_OBJECT (&val));
    } else {
      SWFDEC_INFO ("\"TextField\" is not an object");
    }
  }

  swfdec_text_field_movie_init_properties (cx);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}
