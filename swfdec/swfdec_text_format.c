/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_as_array.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_player_internal.h"
/* for getTextExtent */
#include "swfdec_text_buffer.h"
#include "swfdec_text_layout.h"

G_DEFINE_TYPE (SwfdecTextFormat, swfdec_text_format, SWFDEC_TYPE_AS_RELAY)

static int property_offsets[] = {
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.align),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.block_indent),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.bold),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.bullet),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.color),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.display),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.font),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.indent),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.italic),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.kerning),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.leading),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.left_margin),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.letter_spacing),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.right_margin),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.size),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.tab_stops),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.target),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.underline),
  G_STRUCT_OFFSET (SwfdecTextFormat, attr.url)
};

static void
swfdec_text_format_mark (SwfdecGcObject *object)
{
  SwfdecTextFormat *format = SWFDEC_TEXT_FORMAT (object);

  swfdec_text_attributes_mark (&format->attr);

  SWFDEC_GC_OBJECT_CLASS (swfdec_text_format_parent_class)->mark (object);
}

static void
swfdec_text_format_dispose (GObject *object)
{
  SwfdecTextFormat *format = SWFDEC_TEXT_FORMAT (object);

  swfdec_text_attributes_reset (&format->attr);

  G_OBJECT_CLASS (swfdec_text_format_parent_class)->dispose (object);
}

static void
swfdec_text_format_class_init (SwfdecTextFormatClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_text_format_dispose;

  gc_class->mark = swfdec_text_format_mark;
}

static void
swfdec_text_format_init (SwfdecTextFormat *format)
{
  swfdec_text_attributes_reset (&format->attr);
}

static void
swfdec_text_format_get_string (SwfdecAsObject *object,
    SwfdecTextAttribute property, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, property)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (ret,
      G_STRUCT_MEMBER (const char *, format, property_offsets[property]));
}

static void
swfdec_text_format_set_string (SwfdecAsObject *object,
    SwfdecTextAttribute property, guint argc, SwfdecAsValue *argv)
{
  SwfdecTextFormat *format;
  SwfdecAsContext *context;
  const char *s;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  context = swfdec_gc_object_get_context (format);
  swfdec_as_value_to_integer (context, &argv[0]);
  swfdec_as_value_to_number (context, &argv[0]);
  s = swfdec_as_value_to_string (context, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    /* FIXME: reset to defaults here? */
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, property);
  } else {
    G_STRUCT_MEMBER (const char *, format, property_offsets[property]) = s;
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, property);
  }
  /* FIXME: figure out what to do here */
}

static void
swfdec_text_format_get_boolean (SwfdecAsObject *object,
    SwfdecTextAttribute property, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, property)) {
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
    SwfdecTextAttribute property, guint argc, SwfdecAsValue *argv)
{
  SwfdecTextFormat *format;
  SwfdecAsContext *context;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  context = swfdec_gc_object_get_context (format);
  swfdec_as_value_to_integer (context, &argv[0]);
  swfdec_as_value_to_number (context, &argv[0]);
  swfdec_as_value_to_string (context, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, property);
  } else {
    G_STRUCT_MEMBER (gboolean, format, property_offsets[property]) =
      swfdec_as_value_to_boolean (context, &argv[0]);
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, property);
  }
}

static void
swfdec_text_format_get_integer (SwfdecAsObject *object,
    SwfdecTextAttribute property, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, property)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  swfdec_as_value_set_number (swfdec_gc_object_get_context (object), ret,
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
    SwfdecTextAttribute property, guint argc, SwfdecAsValue *argv,
    gboolean allow_negative)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, property);
  } else {
    G_STRUCT_MEMBER (int, format, property_offsets[property]) =
      swfdec_text_format_value_to_integer (swfdec_gc_object_get_context (format),
	  &argv[0], allow_negative);
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, property);
  }
}

static void
swfdec_text_format_do_get_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_ALIGN)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  switch (format->attr.align) {
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

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_as_value_to_number (cx, &argv[0]);
  s = swfdec_as_value_to_string (cx, &argv[0]);

  if (!g_ascii_strcasecmp (s, "left")) {
    format->attr.align = SWFDEC_TEXT_ALIGN_LEFT;
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_ALIGN);
  } else if (!g_ascii_strcasecmp (s, "right")) {
    format->attr.align = SWFDEC_TEXT_ALIGN_RIGHT;
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_ALIGN);
  } else if (!g_ascii_strcasecmp (s, "center")) {
    format->attr.align = SWFDEC_TEXT_ALIGN_CENTER;
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_ALIGN);
  } else if (!g_ascii_strcasecmp (s, "justify")) {
    format->attr.align = SWFDEC_TEXT_ALIGN_JUSTIFY;
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_ALIGN);
  }
}

static void
swfdec_text_format_do_get_block_indent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT, ret);
}

static void
swfdec_text_format_do_set_block_indent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT, argc, argv,
      cx->version >= 8);
}

static void
swfdec_text_format_do_get_bold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_ATTRIBUTE_BOLD, ret);
}

static void
swfdec_text_format_do_set_bold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_ATTRIBUTE_BOLD, argc, argv);
}

static void
swfdec_text_format_do_get_bullet (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_ATTRIBUTE_BULLET, ret);
}

static void
swfdec_text_format_do_set_bullet (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_ATTRIBUTE_BULLET, argc, argv);
}

static void
swfdec_text_format_do_get_color (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_COLOR)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  swfdec_as_value_set_number (cx, ret, format->attr.color);
}

static void
swfdec_text_format_do_set_color (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_COLOR);
  } else {
    format->attr.color = (unsigned) swfdec_as_value_to_integer (cx, &argv[0]);
    swfdec_as_value_to_integer (cx, &argv[0]);
    swfdec_as_value_to_string (cx, &argv[0]);

    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_COLOR);
  }
}

static void
swfdec_text_format_do_get_display (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_DISPLAY))
  {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  switch (format->attr.display) {
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

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_as_value_to_number (cx, &argv[0]);
  swfdec_as_value_to_string (cx, &argv[0]);
  s = swfdec_as_value_to_string (cx, &argv[0]); // oh yes, let's call it twice

  if (!g_ascii_strcasecmp (s, "none")) {
    format->attr.display = SWFDEC_TEXT_DISPLAY_NONE;
  } else if (!g_ascii_strcasecmp (s, "inline")) {
    format->attr.display = SWFDEC_TEXT_DISPLAY_INLINE;
  } else {
    format->attr.display = SWFDEC_TEXT_DISPLAY_BLOCK;
  }

  SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_DISPLAY);
}

static void
swfdec_text_format_do_get_font (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object, SWFDEC_TEXT_ATTRIBUTE_FONT, ret);
}

static void
swfdec_text_format_do_set_font (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object, SWFDEC_TEXT_ATTRIBUTE_FONT, argc, argv);
}

static void
swfdec_text_format_do_get_indent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_ATTRIBUTE_INDENT, ret);
}

static void
swfdec_text_format_do_set_indent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_ATTRIBUTE_INDENT, argc, argv,
      cx->version >= 8);
}

static void
swfdec_text_format_do_get_italic (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_ATTRIBUTE_ITALIC, ret);
}

static void
swfdec_text_format_do_set_italic (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_ATTRIBUTE_ITALIC, argc, argv);
}

static void
swfdec_text_format_do_get_kerning (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_ATTRIBUTE_KERNING, ret);
}

static void
swfdec_text_format_do_set_kerning (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_ATTRIBUTE_KERNING, argc, argv);
}

static void
swfdec_text_format_do_get_leading (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_ATTRIBUTE_LEADING, ret);
}

static void
swfdec_text_format_do_set_leading (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_ATTRIBUTE_LEADING, argc, argv,
      cx->version >= 8);
}

static void
swfdec_text_format_do_get_left_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN, ret);
}

static void
swfdec_text_format_do_set_left_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN, argc, argv, FALSE);
}

static void
swfdec_text_format_do_get_letter_spacing (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  swfdec_as_value_set_number (cx, ret, format->attr.letter_spacing);
}

static void
swfdec_text_format_do_set_letter_spacing (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  double d;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (cx, &argv[0]);
  d = swfdec_as_value_to_number (cx, &argv[0]);
  swfdec_as_value_to_string (cx, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]))
  {
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set,
	SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING);
  }
  else
  {
    format->attr.letter_spacing = d;
    SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set,
	SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING);
  }
}

static void
swfdec_text_format_do_get_right_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN, ret);
}

static void
swfdec_text_format_do_set_right_margin (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN, argc, argv,
      FALSE);
}

static void
swfdec_text_format_do_get_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_integer (object, SWFDEC_TEXT_ATTRIBUTE_SIZE, ret);
}

static void
swfdec_text_format_do_set_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_integer (object, SWFDEC_TEXT_ATTRIBUTE_SIZE, argc, argv, TRUE);
}

static void
swfdec_text_format_do_get_tab_stops (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  guint i;
  SwfdecAsValue val;
  SwfdecAsObject *array;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS)) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  array = swfdec_as_array_new (cx);
  for (i = 0; i < format->attr.n_tab_stops; i++) {
    swfdec_as_value_set_integer (cx, &val, format->attr.tab_stops[i]);
    swfdec_as_array_push (array, &val);
  }
  SWFDEC_AS_VALUE_SET_COMPOSITE (ret, array);
}

static void
swfdec_text_format_do_set_tab_stops (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  if (object == NULL || !SWFDEC_IS_TEXT_FORMAT (object->relay))
    return;
  format = SWFDEC_TEXT_FORMAT (object->relay);

  if (argc < 1)
    return;

  swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_as_value_to_number (cx, &argv[0]);
  swfdec_as_value_to_string (cx, &argv[0]);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]))
  {
    g_free (format->attr.tab_stops);
    format->attr.tab_stops = NULL;
    format->attr.n_tab_stops = 0;
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS);
  }
  else if (SWFDEC_AS_VALUE_IS_COMPOSITE (&argv[0]) &&
	SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[0])->array)
  {
    SwfdecAsObject *array;
    SwfdecAsValue val;
    guint i;
    int len;

    array = SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[0]);
    len = swfdec_as_array_get_length (array);

    if (!SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS)) {
      // special case, if we have null and array is empty, keep it at null
      if (len == 0)
	return;
      SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS);
    }

    g_free (format->attr.tab_stops);
    format->attr.n_tab_stops = MAX (0, len);
    format->attr.tab_stops = g_new (guint, format->attr.n_tab_stops);
    for (i = 0; i < format->attr.n_tab_stops; i++) {
      swfdec_as_array_get_value (array, i, &val);
      format->attr.tab_stops[i] = swfdec_text_format_value_to_integer (cx, &val, TRUE);
    }
  }
  else if (SWFDEC_AS_VALUE_IS_STRING (&argv[0]))
  {
    gsize i;

    // special case: empty strings mean null
    if (SWFDEC_AS_VALUE_GET_STRING (&argv[0]) == SWFDEC_AS_STR_EMPTY) {
      g_free (format->attr.tab_stops);
      format->attr.tab_stops = NULL;
      format->attr.n_tab_stops = 0;
      SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS);
    } else {
      int n = cx->version >= 8 ? G_MININT : 0;
      SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS);
      format->attr.n_tab_stops = strlen (SWFDEC_AS_VALUE_GET_STRING (&argv[0]));
      format->attr.tab_stops = g_new (guint, format->attr.n_tab_stops);
      for (i = 0; i < format->attr.n_tab_stops; i++) {
	format->attr.tab_stops[i] = n;
      }
    }
  }
  else if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS))
  {
    format->attr.n_tab_stops = 0;
    g_free (format->attr.tab_stops);
    format->attr.tab_stops = NULL;
  }
}

static void
swfdec_text_format_do_get_target (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object, SWFDEC_TEXT_ATTRIBUTE_TARGET, ret);
}

static void
swfdec_text_format_do_set_target (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object, SWFDEC_TEXT_ATTRIBUTE_TARGET, argc, argv);
}

static void
swfdec_text_format_do_get_underline (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_get_boolean (object, SWFDEC_TEXT_ATTRIBUTE_UNDERLINE, ret);
}

static void
swfdec_text_format_do_set_underline (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  swfdec_text_format_set_boolean (object, SWFDEC_TEXT_ATTRIBUTE_UNDERLINE, argc, argv);
}

static void
swfdec_text_format_do_get_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object, SWFDEC_TEXT_ATTRIBUTE_URL, ret);
}

static void
swfdec_text_format_do_set_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object, SWFDEC_TEXT_ATTRIBUTE_URL, argc, argv);
}

static void
swfdec_text_format_getTextExtent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  SwfdecTextBuffer *buffer;
  SwfdecTextLayout *layout;
  SwfdecAsObject *obj;
  SwfdecAsValue val;
  const char* text;
  int i, j;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXT_FORMAT, &format, "s", &text);

  obj = swfdec_as_object_new_empty (cx);

  buffer = swfdec_text_buffer_new ();
  swfdec_text_buffer_set_default_attributes (buffer,
      &format->attr, format->values_set);
  swfdec_text_buffer_append_text (buffer, text);
  layout = swfdec_text_layout_new (buffer);
  
  i = swfdec_text_layout_get_width (layout);
  swfdec_as_value_set_integer (cx, &val, i);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_width, &val);
  if (i)
    i += 4;
  swfdec_as_value_set_integer (cx, &val, i);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_textFieldWidth, &val);

  i = swfdec_text_layout_get_height (layout);
  swfdec_as_value_set_integer (cx, &val, i);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_height, &val);
  if (i)
    i += 4;
  swfdec_as_value_set_integer (cx, &val, i);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_textFieldHeight, &val);

  swfdec_text_layout_get_ascent_descent (layout, &i, &j);
  swfdec_as_value_set_integer (cx, &val, i);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_ascent, &val);
  swfdec_as_value_set_integer (cx, &val, j);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_descent, &val);

  SWFDEC_AS_VALUE_SET_COMPOSITE (ret, obj);
  g_object_unref (layout);
  g_object_unref (buffer);
}

void
swfdec_text_format_add (SwfdecTextFormat *format, const SwfdecTextFormat *from)
{
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (format));
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (from));

  swfdec_text_attributes_copy (&format->attr, &from->attr, from->values_set);
  format->values_set |= from->values_set;
}

void
swfdec_text_format_remove_different (SwfdecTextFormat *format,
    const SwfdecTextFormat *from)
{
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (format));
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (from));

  format->values_set &= ~swfdec_text_attributes_diff (&format->attr, &from->attr);
}

gboolean
swfdec_text_format_equal_or_undefined (const SwfdecTextFormat *a,
    const SwfdecTextFormat *b)
{
  int set, diff;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (a), FALSE);
  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (b), FALSE);

  set = a->values_set & b->values_set;
  diff = swfdec_text_attributes_diff (&a->attr, &b->attr);

  return (set & diff) == 0;
}

gboolean
swfdec_text_format_equal (const SwfdecTextFormat *a, const SwfdecTextFormat *b)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (a), FALSE);
  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (b), FALSE);

  if (a->values_set != b->values_set)
    return FALSE;

  return (a->values_set & swfdec_text_attributes_diff (&a->attr, &b->attr)) == 0;
}

void
swfdec_text_format_set_defaults (SwfdecTextFormat *format)
{
  swfdec_text_attributes_reset (&format->attr);
  format->values_set = SWFDEC_TEXT_ATTRIBUTES_MASK;

  if (swfdec_gc_object_get_context (format)->version < 8) {
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_KERNING);
    SWFDEC_TEXT_ATTRIBUTE_UNSET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING);
  }
}

static void
swfdec_text_format_clear (SwfdecTextFormat *format)
{
  format->values_set = 0;

  format->attr.display = SWFDEC_TEXT_DISPLAY_BLOCK;
  SWFDEC_TEXT_ATTRIBUTE_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_DISPLAY);
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
  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_COMPOSITE (&val);
  swfdec_as_object_get_variable (proto, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_COMPOSITE (&val);

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

SWFDEC_AS_NATIVE (110, 0, swfdec_text_format_construct)
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
  SwfdecTextFormat *format;
  SwfdecAsFunction *function;
  SwfdecAsValue val;
  guint i;

  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  swfdec_text_format_init_properties (cx);

  format = g_object_new (SWFDEC_TYPE_TEXT_FORMAT, "context", cx, NULL);
  swfdec_text_format_clear (format);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (format));

  function = swfdec_as_native_function_new_bare (cx, 
	SWFDEC_AS_STR_getTextExtent, swfdec_text_format_getTextExtent, NULL);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (function)));
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_getTextExtent, &val);

  for (i = 0; i < argc && arguments[i] != NULL; i++) {
    swfdec_as_object_set_variable (object, arguments[i], &argv[i]);
  }
}

SwfdecTextFormat *
swfdec_text_format_copy (SwfdecTextFormat *copy_from)
{
  SwfdecTextFormat *copy_to;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FORMAT (copy_from), NULL);

  copy_to = swfdec_text_format_new_no_properties (
      swfdec_gc_object_get_context (copy_from));

  swfdec_text_attributes_copy (&copy_to->attr, &copy_from->attr, -1);
  copy_to->values_set = copy_from->values_set;

  return copy_to;
}

SwfdecTextFormat *
swfdec_text_format_new_no_properties (SwfdecAsContext *context)
{
  SwfdecAsObject *object;
  SwfdecTextFormat *ret;
  SwfdecAsFunction *function;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  ret = g_object_new (SWFDEC_TYPE_TEXT_FORMAT, "context", context, NULL);

  swfdec_text_format_clear (ret);
  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_constructor_by_name (object, SWFDEC_AS_STR_TextFormat, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (ret));

  // FIXME: Need better way to create function without prototype/constructor
  function = swfdec_as_native_function_new_bare (context, 
	SWFDEC_AS_STR_getTextExtent, swfdec_text_format_getTextExtent, NULL);
  SWFDEC_AS_VALUE_SET_COMPOSITE (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (function)));
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_getTextExtent, &val);

  return ret;
}

SwfdecTextFormat *
swfdec_text_format_new (SwfdecAsContext *context)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  swfdec_text_format_init_properties (context);

  return swfdec_text_format_new_no_properties (context);
}
