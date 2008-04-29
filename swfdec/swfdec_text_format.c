/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <pango/pangocairo.h>

#include "swfdec_text_format.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_array.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecTextFormat, swfdec_text_format, SWFDEC_TYPE_AS_OBJECT)

static int property_offsets[] = {
  G_STRUCT_OFFSET (SwfdecTextFormat, align),
  G_STRUCT_OFFSET (SwfdecTextFormat, block_indent),
  G_STRUCT_OFFSET (SwfdecTextFormat, bold),
  G_STRUCT_OFFSET (SwfdecTextFormat, bullet),
  G_STRUCT_OFFSET (SwfdecTextFormat, color),
  G_STRUCT_OFFSET (SwfdecTextFormat, display),
  G_STRUCT_OFFSET (SwfdecTextFormat, font),
  G_STRUCT_OFFSET (SwfdecTextFormat, indent),
  G_STRUCT_OFFSET (SwfdecTextFormat, italic),
  G_STRUCT_OFFSET (SwfdecTextFormat, kerning),
  G_STRUCT_OFFSET (SwfdecTextFormat, leading),
  G_STRUCT_OFFSET (SwfdecTextFormat, left_margin),
  G_STRUCT_OFFSET (SwfdecTextFormat, letter_spacing),
  G_STRUCT_OFFSET (SwfdecTextFormat, right_margin),
  G_STRUCT_OFFSET (SwfdecTextFormat, size),
  G_STRUCT_OFFSET (SwfdecTextFormat, tab_stops),
  G_STRUCT_OFFSET (SwfdecTextFormat, target),
  G_STRUCT_OFFSET (SwfdecTextFormat, underline),
  G_STRUCT_OFFSET (SwfdecTextFormat, url)
};

static void
swfdec_text_format_do_mark (SwfdecAsObject *object)
{
  SwfdecTextFormat *format = SWFDEC_TEXT_FORMAT (object);

  if (format->font != NULL)
    swfdec_as_string_mark (format->font);
  if (format->tab_stops != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (format->tab_stops));
  if (format->target != NULL)
    swfdec_as_string_mark (format->target);
  if (format->url != NULL)
    swfdec_as_string_mark (format->url);

  SWFDEC_AS_OBJECT_CLASS (swfdec_text_format_parent_class)->mark (object);
}

static void
swfdec_text_format_class_init (SwfdecTextFormatClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_text_format_do_mark;
}

static void
swfdec_text_format_init (SwfdecTextFormat *text_format)
{
}

static gboolean
swfdec_text_format_is_set (const SwfdecTextFormat *format, SwfdecTextFormatProperty property)
{
  return (format->values_set & (1 << property));
}

static void
swfdec_text_format_mark_set (SwfdecTextFormat *format,
    SwfdecTextFormatProperty property)
{
  format->values_set |= (1 << property);
}

static void
swfdec_text_format_mark_unset (SwfdecTextFormat *format,
    SwfdecTextFormatProperty property)
{
  format->values_set &= ~(1 << property);
}

static void
swfdec_text_format_get_string (SwfdecAsObject *object,
    SwfdecTextFormatProperty property, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, property)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (ret,
      G_STRUCT_MEMBER (const char *, format, property_offsets[property]));
}

static void
swfdec_text_format_set_string (SwfdecAsObject *object,
    SwfdecTextFormatProperty property, guint argc, SwfdecAsValue *argv)
{
  SwfdecTextFormat *format;
  const char *s;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (object->context, &argv[0]);
  swfdec_as_value_to_number (object->context, &argv[0]);
  s = swfdec_as_value_to_string (object->context, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    G_STRUCT_MEMBER (const char *, format, property_offsets[property]) = NULL;
    swfdec_text_format_mark_unset (format, property);
  } else {
    G_STRUCT_MEMBER (const char *, format, property_offsets[property]) = s;
    swfdec_text_format_mark_set (format, property);
  }
}

static void
swfdec_text_format_get_boolean (SwfdecAsObject *object,
    SwfdecTextFormatProperty property, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, property)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  if (G_STRUCT_MEMBER (gboolean, format, property_offsets[property])) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
  }
}

static void
swfdec_text_format_set_boolean (SwfdecAsObject *object,
    SwfdecTextFormatProperty property, guint argc, SwfdecAsValue *argv)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (object->context, &argv[0]);
  swfdec_as_value_to_number (object->context, &argv[0]);
  swfdec_as_value_to_string (object->context, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    swfdec_text_format_mark_unset (format, property);
  } else {
    G_STRUCT_MEMBER (gboolean, format, property_offsets[property]) =
      swfdec_as_value_to_boolean (object->context, &argv[0]);
    swfdec_text_format_mark_set (format, property);
  }
}

static void
swfdec_text_format_get_integer (SwfdecAsObject *object,
    SwfdecTextFormatProperty property, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, property)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_NUMBER (ret,
      (double)G_STRUCT_MEMBER (int, format, property_offsets[property]));
}

static int
swfdec_text_format_value_to_integer (SwfdecAsContext *cx, SwfdecAsValue *val,
    gboolean allow_negative)
{
  double d;
  int n;

  n = swfdec_as_value_to_integer (cx, val);
  d = swfdec_as_value_to_number (cx, val);
  swfdec_as_value_to_string (cx, val);

  if (cx->version >= 8) {
    if (isnan (d))
      return (allow_negative ? G_MININT32 : 0);

    if (!isfinite (d)) {
      if (d > 0) {
	return G_MININT32;
      } else {
	return (allow_negative ? G_MININT32 : 0);
      }
    }
    if (d > (double)G_MAXINT32)
      return G_MININT32;

    n = (int)d;
    if (!allow_negative && n < 0) {
      return 0;
    } else {
      return n;
    }
  } else {
    if (!allow_negative && n < 0) {
      return 0;
    } else {
      return n;
    }
  }
}

static void
swfdec_text_format_set_integer (SwfdecAsObject *object,
    SwfdecTextFormatProperty property, guint argc, SwfdecAsValue *argv,
    gboolean allow_negative)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    swfdec_text_format_mark_unset (format, property);
  } else {
    G_STRUCT_MEMBER (int, format, property_offsets[property]) =
      swfdec_text_format_value_to_integer (object->context, &argv[0],
	  allow_negative);
    swfdec_text_format_mark_set (format, property);
  }
}

static void
swfdec_text_format_do_get_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_ALIGN)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  switch (format->align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_left);
      break;
    case SWFDEC_TEXT_ALIGN_RIGHT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_right);
      break;
    case SWFDEC_TEXT_ALIGN_CENTER:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_center);
      break;
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_justify);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_format_do_set_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  const char *s;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_as_value_to_number (cx, &argv[0]);
  s = swfdec_as_value_to_string (cx, &argv[0]);

  if (!g_ascii_strcasecmp (s, "left")) {
    format->align = SWFDEC_TEXT_ALIGN_LEFT;
    swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_ALIGN);
  } else if (!g_ascii_strcasecmp (s, "right")) {
    format->align = SWFDEC_TEXT_ALIGN_RIGHT;
    swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_ALIGN);
  } else if (!g_ascii_strcasecmp (s, "center")) {
    format->align = SWFDEC_TEXT_ALIGN_CENTER;
    swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_ALIGN);
  } else if (!g_ascii_strcasecmp (s, "justify")) {
    format->align = SWFDEC_TEXT_ALIGN_JUSTIFY;
    swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_ALIGN);
  }
}

static void
swfdec_text_format_do_get_block_indent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_FORMAT_BLOCK_INDENT, ret);
}

static void
swfdec_text_format_do_set_block_indent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_FORMAT_BLOCK_INDENT, argc, argv,
      cx->version >= 8);
}

static void
swfdec_text_format_do_get_bold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_FORMAT_BOLD, ret);
}

static void
swfdec_text_format_do_set_bold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_FORMAT_BOLD, argc, argv);
}

static void
swfdec_text_format_do_get_bullet (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_FORMAT_BULLET, ret);
}

static void
swfdec_text_format_do_set_bullet (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_FORMAT_BULLET, argc, argv);
}

static void
swfdec_text_format_do_get_color (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_COLOR)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_NUMBER (ret, format->color);
}

static void
swfdec_text_format_do_set_color (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    swfdec_text_format_mark_unset (format, SWFDEC_TEXT_FORMAT_COLOR);
  } else {
    format->color = (unsigned) swfdec_as_value_to_integer (cx, &argv[0]);
    swfdec_as_value_to_integer (cx, &argv[0]);
    swfdec_as_value_to_string (cx, &argv[0]);

    swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_COLOR);
  }
}

static void
swfdec_text_format_do_get_display (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_DISPLAY))
  {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  switch (format->display) {
    case SWFDEC_TEXT_DISPLAY_NONE:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_none);
      break;
    case SWFDEC_TEXT_DISPLAY_INLINE:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_inline);
      break;
    case SWFDEC_TEXT_DISPLAY_BLOCK:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_block);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_format_do_set_display (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  const char *s;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_as_value_to_number (cx, &argv[0]);
  swfdec_as_value_to_string (cx, &argv[0]);
  s = swfdec_as_value_to_string (cx, &argv[0]); // oh yes, let's call it twice

  if (!g_ascii_strcasecmp (s, "none")) {
    format->display = SWFDEC_TEXT_DISPLAY_NONE;
  } else if (!g_ascii_strcasecmp (s, "inline")) {
    format->display = SWFDEC_TEXT_DISPLAY_INLINE;
  } else {
    format->display = SWFDEC_TEXT_DISPLAY_BLOCK;
  }

  swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_DISPLAY);
}

static void
swfdec_text_format_do_get_font (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object, SWFDEC_TEXT_FORMAT_FONT, ret);
}

static void
swfdec_text_format_do_set_font (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object, SWFDEC_TEXT_FORMAT_FONT, argc, argv);
}

static void
swfdec_text_format_do_get_indent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_FORMAT_INDENT, ret);
}

static void
swfdec_text_format_do_set_indent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_FORMAT_INDENT, argc, argv,
      cx->version >= 8);
}

static void
swfdec_text_format_do_get_italic (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_FORMAT_ITALIC, ret);
}

static void
swfdec_text_format_do_set_italic (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_FORMAT_ITALIC, argc, argv);
}

static void
swfdec_text_format_do_get_kerning (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_FORMAT_KERNING, ret);
}

static void
swfdec_text_format_do_set_kerning (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_FORMAT_KERNING, argc, argv);
}

static void
swfdec_text_format_do_get_leading (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_FORMAT_LEADING, ret);
}

static void
swfdec_text_format_do_set_leading (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_FORMAT_LEADING, argc, argv,
      cx->version >= 8);
}

static void
swfdec_text_format_do_get_left_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_FORMAT_LEFT_MARGIN, ret);
}

static void
swfdec_text_format_do_set_left_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_FORMAT_LEFT_MARGIN, argc, argv, FALSE);
}

static void
swfdec_text_format_do_get_letter_spacing (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_LETTER_SPACING)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_NUMBER (ret, format->letter_spacing);
}

static void
swfdec_text_format_do_set_letter_spacing (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  double d;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (cx, &argv[0]);
  d = swfdec_as_value_to_number (cx, &argv[0]);
  swfdec_as_value_to_string (cx, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]))
  {
    swfdec_text_format_mark_unset (format,
	SWFDEC_TEXT_FORMAT_LETTER_SPACING);
  }
  else
  {
    format->letter_spacing = d;
    swfdec_text_format_mark_set (format,
	SWFDEC_TEXT_FORMAT_LETTER_SPACING);
  }
}

static void
swfdec_text_format_do_get_right_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_FORMAT_RIGHT_MARGIN, ret);
}

static void
swfdec_text_format_do_set_right_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_FORMAT_RIGHT_MARGIN, argc, argv,
      FALSE);
}

static void
swfdec_text_format_do_get_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_FORMAT_SIZE, ret);
}

static void
swfdec_text_format_do_set_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_FORMAT_SIZE, argc, argv, TRUE);
}

static void
swfdec_text_format_do_get_tab_stops (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (!swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_TAB_STOPS)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (format->tab_stops));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (format->tab_stops));
}

static void
swfdec_text_format_do_set_tab_stops (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXT_FORMAT (object))
    return;
  format = SWFDEC_TEXT_FORMAT (object);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_as_value_to_number (cx, &argv[0]);
  swfdec_as_value_to_string (cx, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]))
  {
    format->tab_stops = NULL;
    swfdec_text_format_mark_unset (format, SWFDEC_TEXT_FORMAT_TAB_STOPS);
  }
  else if (SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]) &&
	SWFDEC_IS_AS_ARRAY (SWFDEC_AS_VALUE_GET_OBJECT (&argv[0])))
  {
    SwfdecAsArray *array;
    SwfdecAsValue val;
    gint32 len, i;
    int n;

    array = SWFDEC_AS_ARRAY (SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]));
    len = swfdec_as_array_get_length (array);

    if (!swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_TAB_STOPS)) {
      // special case, if we have null and array is empty, keep it at null
      if (len == 0)
	return;
      format->tab_stops = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
      if (!format->tab_stops)
	return;
      swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_TAB_STOPS);
    }

    swfdec_as_array_set_length (format->tab_stops, 0);
    for (i = 0; i < len; i++) {
      swfdec_as_array_get_value (array, i, &val);
      n = swfdec_text_format_value_to_integer (cx, &val, TRUE);
      SWFDEC_AS_VALUE_SET_INT (&val, n);
      swfdec_as_array_set_value (format->tab_stops, i, &val);
    }
  }
  else if (SWFDEC_AS_VALUE_IS_STRING (&argv[0]))
  {
    gsize i, len;
    SwfdecAsValue val;

    len = strlen (SWFDEC_AS_VALUE_GET_STRING (&argv[0]));

    // special case: empty strings mean null
    if (len == 0) {
      format->tab_stops = NULL;
      swfdec_text_format_mark_unset (format,
	  SWFDEC_TEXT_FORMAT_TAB_STOPS);
    } else {
      format->tab_stops = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
      if (format->tab_stops != NULL) {
	swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_TAB_STOPS);
	if (cx->version >= 8) {
	  SWFDEC_AS_VALUE_SET_INT (&val, -2147483648);
	} else {
	  SWFDEC_AS_VALUE_SET_INT (&val, 0);
	}
	for (i = 0; i < len; i++) {
	  swfdec_as_array_push (format->tab_stops, &val);
	}
      } else {
	swfdec_text_format_mark_unset (format, SWFDEC_TEXT_FORMAT_TAB_STOPS);
      }
    }
  }
  else if (swfdec_text_format_is_set (format, SWFDEC_TEXT_FORMAT_TAB_STOPS))
  {
    swfdec_as_array_set_length (format->tab_stops, 0);
    swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_TAB_STOPS);
  }
}

static void
swfdec_text_format_do_get_target (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object, SWFDEC_TEXT_FORMAT_TARGET, ret);
}

static void
swfdec_text_format_do_set_target (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object, SWFDEC_TEXT_FORMAT_TARGET, argc, argv);
}

static void
swfdec_text_format_do_get_underline (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_FORMAT_UNDERLINE, ret);
}

static void
swfdec_text_format_do_set_underline (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_FORMAT_UNDERLINE, argc, argv);
}

static void
swfdec_text_format_do_get_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object, SWFDEC_TEXT_FORMAT_URL, ret);
}

static void
swfdec_text_format_do_set_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object, SWFDEC_TEXT_FORMAT_URL, argc, argv);
}

static void
swfdec_text_format_getTextExtent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  SwfdecAsObject *obj = swfdec_as_object_new_empty (cx);
  SwfdecAsValue val;
  const gchar *text;
  gint height = 0, width = 0,  text_field_width = 0, text_field_height = 0;
  gint ascent = 0, descent = 0;
  GList *item_list;
  PangoAnalysis analysis;
  PangoFontMap *fontmap;
  PangoContext *pcontext;
  PangoAttrList *attrs;
  PangoGlyphString *glyph_string = pango_glyph_string_new();
  PangoRectangle ink_rect;
  PangoFontDescription *desc;
  PangoFont *font;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FORMAT, &format, "s", &text);

  /* If the string is empty we'll just leave all the values as 0. */
  if (text != SWFDEC_AS_STR_EMPTY){
    fontmap = pango_cairo_font_map_get_default ();
    pcontext =
      pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

    attrs = pango_attr_list_new ();
    item_list = pango_itemize (pcontext, text, 0, strlen (text), attrs, NULL);
    pango_attr_list_unref (attrs);

    analysis = ((PangoItem *)(item_list[0].data))->analysis;
    pango_shape (text, strlen (text), &analysis, glyph_string);

    desc = pango_font_description_new ();
    if (format->font == NULL)
      pango_font_description_set_family_static (desc, SWFDEC_AS_STR_Times_New_Roman);
    else
      pango_font_description_set_family_static (desc, format->font);
    pango_font_description_set_size (desc, format->size * PANGO_SCALE);
    if (format->bold){
      pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    }
    if (format->italic)
      pango_font_description_set_style (desc, PANGO_STYLE_ITALIC);

    font = pango_font_map_load_font (fontmap, pcontext, desc);
    pango_glyph_string_extents (glyph_string, font, &ink_rect, NULL);

    pango_glyph_string_free (glyph_string);
    g_list_foreach (item_list, (GFunc) pango_item_free, NULL);
    g_list_free (item_list);
    pango_font_description_free (desc);
    g_object_unref (G_OBJECT (pcontext));
    g_object_unref (G_OBJECT (font));

    width = ink_rect.width / PANGO_SCALE + format->left_margin
	+ format->right_margin;
    height = ink_rect.height / PANGO_SCALE + format->leading;
    ascent = PANGO_ASCENT(ink_rect) / PANGO_SCALE;
    descent = PANGO_DESCENT (ink_rect) / PANGO_SCALE;
    text_field_width = ink_rect.width / PANGO_SCALE + format->left_margin
	+ format->right_margin + 4;
    text_field_height = ink_rect.height / PANGO_SCALE + 4 + format->leading;
  }

  SWFDEC_AS_VALUE_SET_INT (&val, width);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_width, &val);

  SWFDEC_AS_VALUE_SET_INT (&val, height);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_height, &val);

  SWFDEC_AS_VALUE_SET_INT (&val, ascent);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_ascent, &val);

  SWFDEC_AS_VALUE_SET_INT (&val, descent);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_descent, &val);

  SWFDEC_AS_VALUE_SET_INT (&val, text_field_width);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_textFieldWidth, &val);

  SWFDEC_AS_VALUE_SET_INT (&val, text_field_height);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_textFieldHeight, &val);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, obj);
}

void
swfdec_text_format_add (SwfdecTextFormat *format, const SwfdecTextFormat *from)
{
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (format));
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (from));

  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_ALIGN))
    format->align = from->align;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_BLOCK_INDENT))
    format->block_indent = from->block_indent;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_BOLD))
    format->bold = from->bold;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_BULLET))
    format->bullet = from->bullet;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_COLOR))
    format->color = from->color;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_DISPLAY))
    format->display = from->display;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_FONT))
    format->font = from->font;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_INDENT))
    format->indent = from->indent;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_ITALIC))
    format->italic = from->italic ;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_KERNING))
    format->kerning = from->kerning;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_LEADING))
    format->leading = from->leading;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_LEFT_MARGIN))
    format->left_margin = from->left_margin;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_LETTER_SPACING))
    format->letter_spacing = from->letter_spacing;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_RIGHT_MARGIN))
    format->right_margin = from->right_margin;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_SIZE))
    format->size = from->size;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_TAB_STOPS))
    format->tab_stops = from->tab_stops;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_TARGET))
    format->target = from->target;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_UNDERLINE))
    format->underline = from->underline;
  if (swfdec_text_format_is_set (from, SWFDEC_TEXT_FORMAT_URL))
    format->url = from->url;

  format->values_set |= from->values_set;
}

void
swfdec_text_format_remove_different (SwfdecTextFormat *format,
    const SwfdecTextFormat *from)
{
  int set;

  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (format));
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (from));

  set = format->values_set & from->values_set;

  if (set & (1 << SWFDEC_TEXT_FORMAT_ALIGN) && format->align != from->align)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_ALIGN);
  if (set & (1 << SWFDEC_TEXT_FORMAT_BLOCK_INDENT) &&
      format->block_indent != from->block_indent) {
    set &= ~(1 << SWFDEC_TEXT_FORMAT_BLOCK_INDENT);
  }
  if (set & (1 << SWFDEC_TEXT_FORMAT_BOLD) && format->bold != from->bold)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_BOLD);
  if (set & (1 << SWFDEC_TEXT_FORMAT_BULLET) && format->bullet != from->bullet)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_BULLET);
  if (set & (1 << SWFDEC_TEXT_FORMAT_COLOR) && format->color != from->color)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_COLOR);
  if (set & (1 << SWFDEC_TEXT_FORMAT_DISPLAY) && format->display != from->display)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_DISPLAY);
  if (set & (1 << SWFDEC_TEXT_FORMAT_FONT) && format->font != from->font)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_FONT);
  if (set & (1 << SWFDEC_TEXT_FORMAT_INDENT) && format->indent != from->indent)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_INDENT);
  if (set & (1 << SWFDEC_TEXT_FORMAT_ITALIC) && format->italic != from->italic)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_ITALIC);
  if (set & (1 << SWFDEC_TEXT_FORMAT_KERNING) && format->kerning != from->kerning)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_KERNING);
  if (set & (1 << SWFDEC_TEXT_FORMAT_LEADING) && format->leading != from->leading)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_LEADING);
  if (set & (1 << SWFDEC_TEXT_FORMAT_LEFT_MARGIN) &&
      format->left_margin != from->left_margin) {
    set &= ~(1 << SWFDEC_TEXT_FORMAT_LEFT_MARGIN);
  }
  if (set & (1 << SWFDEC_TEXT_FORMAT_LETTER_SPACING) &&
      format->letter_spacing != from->letter_spacing) {
    set &= ~(1 << SWFDEC_TEXT_FORMAT_LETTER_SPACING);
  }
  if (set & (1 << SWFDEC_TEXT_FORMAT_RIGHT_MARGIN) &&
      format->right_margin != from->right_margin) {
    set &= ~(1 << SWFDEC_TEXT_FORMAT_RIGHT_MARGIN);
  }
  if (set & (1 << SWFDEC_TEXT_FORMAT_SIZE) && format->size != from->size)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_SIZE);
  if (set & (1 << SWFDEC_TEXT_FORMAT_TAB_STOPS) && format->tab_stops != from->tab_stops)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_TAB_STOPS);
  if (set & (1 << SWFDEC_TEXT_FORMAT_TARGET) && format->target != from->target)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_TARGET);
  if (set & (1 << SWFDEC_TEXT_FORMAT_UNDERLINE) && format->underline != from->underline)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_UNDERLINE);
  if (set & (1 << SWFDEC_TEXT_FORMAT_URL) && format->url != from->url)
    set &= ~(1 << SWFDEC_TEXT_FORMAT_URL);

  format->values_set = set;
}

gboolean
swfdec_text_format_equal_or_undefined (const SwfdecTextFormat *a,
    const SwfdecTextFormat *b)
{
  int set;

  set = a->values_set & b->values_set;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (a), FALSE);
  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (b), FALSE);

  if (set & (1 << SWFDEC_TEXT_FORMAT_ALIGN) && a->align != b->align)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_BLOCK_INDENT) && a->block_indent != b->block_indent)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_BOLD) && a->bold != b->bold)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_BULLET) && a->bullet != b->bullet)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_COLOR) && a->color != b->color)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_DISPLAY) && a->display != b->display)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_FONT) && a->font != b->font)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_INDENT) && a->indent != b->indent)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_ITALIC) && a->italic != b->italic)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_KERNING) && a->kerning != b->kerning)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_LEADING) && a->leading != b->leading)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_LEFT_MARGIN) && a->left_margin != b->left_margin)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_LETTER_SPACING) &&
      a->letter_spacing != b->letter_spacing) {
    return FALSE;
  }
  if (set & (1 << SWFDEC_TEXT_FORMAT_RIGHT_MARGIN) && a->right_margin != b->right_margin)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_SIZE) && a->size != b->size)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_TAB_STOPS) && a->tab_stops != b->tab_stops)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_TARGET) && a->target != b->target)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_UNDERLINE) && a->underline != b->underline)
    return FALSE;
  if (set & (1 << SWFDEC_TEXT_FORMAT_URL) && a->url != b->url)
    return FALSE;

  return TRUE;
}

gboolean
swfdec_text_format_equal (const SwfdecTextFormat *a, const SwfdecTextFormat *b)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (a), FALSE);
  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (b), FALSE);

  if (a->values_set != b->values_set)
    return FALSE;

  return swfdec_text_format_equal_or_undefined (a, b);
}

void
swfdec_text_format_set_defaults (SwfdecTextFormat *format)
{
  format->align = SWFDEC_TEXT_ALIGN_LEFT;
  format->block_indent = 0;
  format->bold = FALSE;
  format->bullet = FALSE;
  format->color = 0;
  format->display = SWFDEC_TEXT_DISPLAY_BLOCK;
  format->font = SWFDEC_AS_STR_Times_New_Roman;
  format->indent = 0;
  format->italic = FALSE;
  format->kerning = FALSE;
  format->leading = 0;
  format->left_margin = 0;
  format->letter_spacing = 0;
  format->right_margin = 0;
  format->size = 12;
  format->tab_stops =
    SWFDEC_AS_ARRAY (swfdec_as_array_new (SWFDEC_AS_OBJECT (format)->context));
  format->target = SWFDEC_AS_STR_EMPTY;
  format->url = SWFDEC_AS_STR_EMPTY;
  format->underline = FALSE;

  format->values_set = (1 << SWFDEC_TEXT_FORMAT_TOTAL) - 1;

  if (SWFDEC_AS_OBJECT (format)->context->version < 8) {
    swfdec_text_format_mark_unset (format, SWFDEC_TEXT_FORMAT_KERNING);
    swfdec_text_format_mark_unset (format, SWFDEC_TEXT_FORMAT_LETTER_SPACING);
  }
}

static void
swfdec_text_format_clear (SwfdecTextFormat *format)
{
  format->font = NULL;
  format->target = NULL;
  format->tab_stops = NULL;
  format->url = NULL;
  format->values_set = 0;

  format->display = SWFDEC_TEXT_DISPLAY_BLOCK;
  swfdec_text_format_mark_set (format, SWFDEC_TEXT_FORMAT_DISPLAY);
}

void
swfdec_text_format_init_properties (SwfdecAsContext *cx)
{
  SwfdecAsValue val;
  SwfdecAsObject *proto;

  // FIXME: We should only initialize if the prototype Object has not been
  // initialized by any object's constructor with native properties
  // (TextField, TextFormat, XML, XMLNode at least)

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextFormat, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  swfdec_as_object_get_variable (proto, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_align,
      swfdec_text_format_do_get_align, swfdec_text_format_do_set_align);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_blockIndent,
      swfdec_text_format_do_get_block_indent,
      swfdec_text_format_do_set_block_indent);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_bold,
      swfdec_text_format_do_get_bold, swfdec_text_format_do_set_bold);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_bullet,
      swfdec_text_format_do_get_bullet, swfdec_text_format_do_set_bullet);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_color,
      swfdec_text_format_do_get_color, swfdec_text_format_do_set_color);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_display,
      swfdec_text_format_do_get_display, swfdec_text_format_do_set_display);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_font,
      swfdec_text_format_do_get_font, swfdec_text_format_do_set_font);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_indent,
      swfdec_text_format_do_get_indent, swfdec_text_format_do_set_indent);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_italic,
      swfdec_text_format_do_get_italic, swfdec_text_format_do_set_italic);
  if (cx->version >= 8) {
    swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_kerning,
	swfdec_text_format_do_get_kerning, swfdec_text_format_do_set_kerning);
  }
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_leading,
      swfdec_text_format_do_get_leading, swfdec_text_format_do_set_leading);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_leftMargin,
      swfdec_text_format_do_get_left_margin,
      swfdec_text_format_do_set_left_margin);
  if (cx->version >= 8) {
    swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_letterSpacing,
	swfdec_text_format_do_get_letter_spacing,
	swfdec_text_format_do_set_letter_spacing);
  }
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_rightMargin,
      swfdec_text_format_do_get_right_margin,
      swfdec_text_format_do_set_right_margin);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_size,
      swfdec_text_format_do_get_size, swfdec_text_format_do_set_size);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_tabStops,
      swfdec_text_format_do_get_tab_stops,
      swfdec_text_format_do_set_tab_stops);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_target,
      swfdec_text_format_do_get_target, swfdec_text_format_do_set_target);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_underline,
      swfdec_text_format_do_get_underline,
      swfdec_text_format_do_set_underline);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_url,
      swfdec_text_format_do_get_url, swfdec_text_format_do_set_url);
}

SWFDEC_AS_CONSTRUCTOR (110, 0, swfdec_text_format_construct, swfdec_text_format_get_type)
void
swfdec_text_format_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  static const char *arguments[] = {
    SWFDEC_AS_STR_font,
    SWFDEC_AS_STR_size,
    SWFDEC_AS_STR_color,
    SWFDEC_AS_STR_bold,
    SWFDEC_AS_STR_italic,
    SWFDEC_AS_STR_underline,
    SWFDEC_AS_STR_url,
    SWFDEC_AS_STR_target,
    SWFDEC_AS_STR_align,
    SWFDEC_AS_STR_leftMargin,
    SWFDEC_AS_STR_rightMargin,
    SWFDEC_AS_STR_indent,
    SWFDEC_AS_STR_leading,
    NULL
  };
  SwfdecAsFunction *function;
  SwfdecAsObject *tmp;
  SwfdecAsValue val;
  guint i;

  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_TEXT_FORMAT (object));

  swfdec_text_format_init_properties (cx);

  swfdec_text_format_clear (SWFDEC_TEXT_FORMAT (object));

  // FIXME: Need better way to create function without prototype/constructor
  tmp = cx->Function;
  cx->Function = NULL;
  function = swfdec_as_native_function_new (cx, SWFDEC_AS_STR_getTextExtent,
      swfdec_text_format_getTextExtent, 0, NULL);
  cx->Function = tmp;
  if (function != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (function));
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_getTextExtent, &val);
  }

  for (i = 0; i < argc && arguments[i] != NULL; i++) {
    swfdec_as_object_set_variable (object, arguments[i], &argv[i]);
  }
}

SwfdecTextFormat *
swfdec_text_format_copy (const SwfdecTextFormat *copy_from)
{
  SwfdecAsObject *object_to;
  SwfdecTextFormat *copy_to;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (copy_from), NULL);

  object_to = swfdec_text_format_new_no_properties (
      SWFDEC_AS_OBJECT (copy_from)->context);
  if (object_to == NULL)
    return NULL;
  copy_to = SWFDEC_TEXT_FORMAT (object_to);

  copy_to->align = copy_from->align;
  copy_to->block_indent = copy_from->block_indent;
  copy_to->bold = copy_from->bold;
  copy_to->bullet = copy_from->bullet;
  copy_to->color = copy_from->color;
  copy_to->display = copy_from->display;
  copy_to->font = copy_from->font;
  copy_to->indent = copy_from->indent;
  copy_to->italic = copy_from->italic ;
  copy_to->kerning = copy_from->kerning;
  copy_to->leading = copy_from->leading;
  copy_to->left_margin = copy_from->left_margin;
  copy_to->letter_spacing = copy_from->letter_spacing;
  copy_to->right_margin = copy_from->right_margin;
  copy_to->size = copy_from->size;
  copy_to->tab_stops = copy_from->tab_stops;
  copy_to->target = copy_from->target;
  copy_to->underline = copy_from->underline;
  copy_to->url = copy_from->url;
  copy_to->values_set = copy_from->values_set;

  return copy_to;
}

SwfdecAsObject *
swfdec_text_format_new_no_properties (SwfdecAsContext *context)
{
  SwfdecAsObject *ret;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecTextFormat)))
    return NULL;

  ret = g_object_new (SWFDEC_TYPE_TEXT_FORMAT, NULL);
  swfdec_as_object_add (ret, context, sizeof (SwfdecTextFormat));

  swfdec_text_format_clear (SWFDEC_TEXT_FORMAT (ret));

  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_TextFormat,
      &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return ret;
  swfdec_as_object_set_constructor (ret, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  return ret;
}

SwfdecAsObject *
swfdec_text_format_new (SwfdecAsContext *context)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  swfdec_text_format_init_properties (context);

  return swfdec_text_format_new_no_properties (context);
}
