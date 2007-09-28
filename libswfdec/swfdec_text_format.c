/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

static void
swfdec_text_format_do_mark (SwfdecAsObject *object)
{
  SwfdecTextFormat *format = SWFDEC_TEXTFORMAT (object);

  if (format->font != NULL)
    swfdec_as_string_mark (format->font);
  // no need to mark letterSpacing, it's always number or null
  if (format->tabStops != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (format->tabStops));
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

static void
swfdec_text_format_get_string (SwfdecAsObject *object, size_t offset,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  const char *value;

  if (!SWFDEC_IS_TEXTFORMAT (object))
    return;

  format = SWFDEC_TEXTFORMAT (object);
  value = G_STRUCT_MEMBER (const char *, format, offset);

  if (value != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, value);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_text_format_set_string (SwfdecAsObject *object, size_t offset,
    guint argc, SwfdecAsValue *argv)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXTFORMAT (object))
    return;
  format = SWFDEC_TEXTFORMAT (object);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    G_STRUCT_MEMBER (const char *, format, offset) = NULL;
  } else {
    G_STRUCT_MEMBER (const char *, format, offset) =
      swfdec_as_value_to_string (object->context, &argv[0]);
  }
}

static void
swfdec_text_format_get_toggle (SwfdecAsObject *object, size_t offset,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  SwfdecToggle value;

  if (!SWFDEC_IS_TEXTFORMAT (object))
    return;

  format = SWFDEC_TEXTFORMAT (object);
  value = G_STRUCT_MEMBER (SwfdecToggle, format, offset);

  switch (value) {
    case SWFDEC_TOGGLE_DISABLED:
      SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
      break;
    case SWFDEC_TOGGLE_ENABLED:
      SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
      break;
    case SWFDEC_TOGGLE_UNDEFINED:
      SWFDEC_AS_VALUE_SET_NULL (ret);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_format_set_toggle (SwfdecAsObject *object, size_t offset,
    guint argc, SwfdecAsValue *argv)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXTFORMAT (object))
    return;
  format = SWFDEC_TEXTFORMAT (object);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    G_STRUCT_MEMBER (SwfdecToggle, format, offset) = SWFDEC_TOGGLE_UNDEFINED;
  } else {
    if (swfdec_as_value_to_boolean (object->context, &argv[0])) {
      G_STRUCT_MEMBER (SwfdecToggle, format, offset) = SWFDEC_TOGGLE_ENABLED;
    } else {
      G_STRUCT_MEMBER (SwfdecToggle, format, offset) = SWFDEC_TOGGLE_DISABLED;
    }
  }
}

static void
swfdec_text_format_get_int (SwfdecAsObject *object, size_t offset,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  double value;

  if (!SWFDEC_IS_TEXTFORMAT (object))
    return;

  format = SWFDEC_TEXTFORMAT (object);
  value = G_STRUCT_MEMBER (double, format, offset);

  if (!isnan (value)) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, value);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static double
swfdec_text_format_value_to_integer (SwfdecAsContext *cx, SwfdecAsValue *val,
    gboolean allow_negative, gboolean is_unsigned)
{
  if (cx->version >= 8) {
    double value = swfdec_as_value_to_number (cx, val);
    if (!is_unsigned && !isnan (value) && !isfinite (value) && value > 0) {
      // don't check allow_negative here
      return -2147483648;
    } else {
      if (is_unsigned) {
	return (unsigned)value;
      } else {
	if (!allow_negative && (int)value < 0) {
	  return 0;
	} else {
	  return (int)value;
	}
      }
    }
  } else {
    int value = swfdec_as_value_to_integer (cx, val);
    if (is_unsigned) {
      return (unsigned)value;
    } else {
      if (!allow_negative && value < 0) {
	return 0;
      } else {
	return value;
      }
    }
  }
}

static void
swfdec_text_format_set_int (SwfdecAsObject *object, size_t offset, guint argc,
    SwfdecAsValue *argv, gboolean allow_negative, gboolean is_unsigned)
{
  SwfdecTextFormat *format;

  if (!SWFDEC_IS_TEXTFORMAT (object))
    return;
  format = SWFDEC_TEXTFORMAT (object);

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0])) {
    G_STRUCT_MEMBER (double, format, offset) = NAN;
  } else {
    G_STRUCT_MEMBER (double, format, offset) =
      swfdec_text_format_value_to_integer (object->context, &argv[0],
	  allow_negative, is_unsigned);
  }
}

static void
swfdec_text_format_get_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

  switch (format->align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_left);
      break;
    case SWFDEC_TEXT_ALIGN_CENTER:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_center);
      break;
    case SWFDEC_TEXT_ALIGN_RIGHT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_right);
      break;
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_justify);
      break;
    case SWFDEC_TEXT_ALIGN_UNDEFINED:
      SWFDEC_AS_VALUE_SET_NULL (ret);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_format_set_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "s", &value);

  if (!g_ascii_strcasecmp (value, "left")) {
    format->align = SWFDEC_TEXT_ALIGN_LEFT;
  } else if (!g_ascii_strcasecmp (value, "center")) {
    format->align = SWFDEC_TEXT_ALIGN_CENTER;
  } else if (!g_ascii_strcasecmp (value, "right")) {
    format->align = SWFDEC_TEXT_ALIGN_RIGHT;
  } else if (!g_ascii_strcasecmp (value, "justify")) {
    format->align = SWFDEC_TEXT_ALIGN_JUSTIFY;
  }
}

static void
swfdec_text_format_get_blockIndent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, blockIndent), ret);
}

static void
swfdec_text_format_set_blockIndent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, blockIndent), argc, argv,
      cx->version >= 8, FALSE);
}

static void
swfdec_text_format_get_bold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, bold), ret);
}

static void
swfdec_text_format_set_bold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, bold), argc, argv);
}

static void
swfdec_text_format_get_bullet (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, bullet), ret);
}

static void
swfdec_text_format_set_bullet (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, bullet), argc, argv);
}

static void
swfdec_text_format_get_color (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, color), ret);
}

static void
swfdec_text_format_set_color (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, color), argc, argv, FALSE, TRUE);
}

static void
swfdec_text_format_get_display (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

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
    case SWFDEC_TEXT_ALIGN_UNDEFINED:
      SWFDEC_AS_VALUE_SET_NULL (ret);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_format_set_display (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "s", &value);

  if (!g_ascii_strcasecmp (value, "none")) {
    format->display = SWFDEC_TEXT_DISPLAY_NONE;
  } else if (!g_ascii_strcasecmp (value, "inline")) {
    format->display = SWFDEC_TEXT_DISPLAY_INLINE;
  } else {
    format->display = SWFDEC_TEXT_DISPLAY_BLOCK;
  }
}

static void
swfdec_text_format_get_font (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, font), ret);
}

static void
swfdec_text_format_set_font (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, font), argc, argv);
}

static void
swfdec_text_format_get_indent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, indent), ret);
}

static void
swfdec_text_format_set_indent (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, indent), argc, argv,
      cx->version >= 8, FALSE);
}

static void
swfdec_text_format_get_italic (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, italic), ret);
}

static void
swfdec_text_format_set_italic (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, italic), argc, argv);
}

static void
swfdec_text_format_get_kerning (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, kerning), ret);
}

static void
swfdec_text_format_set_kerning (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, kerning), argc, argv);
}

static void
swfdec_text_format_get_leading (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, leading), ret);
}

static void
swfdec_text_format_set_leading (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, leading), argc, argv,
      cx->version >= 8, FALSE);
}

static void
swfdec_text_format_get_leftMargin (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, leftMargin), ret);
}

static void
swfdec_text_format_set_leftMargin (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, leftMargin), argc, argv, FALSE, FALSE);
}

static void
swfdec_text_format_get_letterSpacing (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

  *ret = format->letterSpacing;
}

static void
swfdec_text_format_set_letterSpacing (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]))
  {
    SWFDEC_AS_VALUE_SET_NULL (&format->letterSpacing);
  }
  else
  {
    SWFDEC_AS_VALUE_SET_NUMBER (&format->letterSpacing,
	swfdec_as_value_to_number (cx, &argv[0]));
  }
}

static void
swfdec_text_format_get_rightMargin (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, rightMargin), ret);
}

static void
swfdec_text_format_set_rightMargin (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, rightMargin), argc, argv, FALSE,
      FALSE);
}

static void
swfdec_text_format_get_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, size), ret);
}

static void
swfdec_text_format_set_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_int (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, size), argc, argv, TRUE, FALSE);
}

static void
swfdec_text_format_get_tabStops (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

  if (format->tabStops != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (format->tabStops));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_text_format_set_tabStops (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_NULL (&argv[0]))
  {
    format->tabStops = NULL;
  }
  else if (SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]) &&
	SWFDEC_IS_AS_ARRAY (SWFDEC_AS_VALUE_GET_OBJECT (&argv[0])))
  {
    SwfdecAsArray *array;
    SwfdecAsValue val;
    gint32 len, i;
    double d;

    array = SWFDEC_AS_ARRAY (SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]));
    len = swfdec_as_array_get_length (array);

    if (format->tabStops == NULL) {
      // special case, if we have null and array is empty, keep it at null
      if (len == 0)
	return;
      format->tabStops = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
    }

    swfdec_as_array_set_length (format->tabStops, 0);
    for (i = 0; i < len; i++) {
      swfdec_as_array_get_value (array, i, &val);
      d = swfdec_text_format_value_to_integer (cx, &val, TRUE, FALSE);
      SWFDEC_AS_VALUE_SET_NUMBER (&val, d);
      swfdec_as_array_set_value (format->tabStops, i, &val);
    }
  }
  else
  {
    format->tabStops = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
    if (SWFDEC_AS_VALUE_IS_STRING (&argv[0])) {
      size_t i, len;
      SwfdecAsValue val;

      len = strlen (SWFDEC_AS_VALUE_GET_STRING (&argv[0]));

      // special case: empty strings mean null
      if (len == 0) {
	format->tabStops = NULL;
      } else {
	if (cx->version >= 8) {
	  SWFDEC_AS_VALUE_SET_INT (&val, -2147483648);
	} else {
	  SWFDEC_AS_VALUE_SET_INT (&val, 0);
	}
	for (i = 0; i < len; i++) {
	  swfdec_as_array_push (format->tabStops, &val);
	}
      }
    }
  }

}

static void
swfdec_text_format_get_target (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, target), ret);
}

static void
swfdec_text_format_set_target (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, target), argc, argv);
}

static void
swfdec_text_format_get_underline (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, underline), ret);
}

static void
swfdec_text_format_set_underline (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_toggle (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, underline), argc, argv);
}

static void
swfdec_text_format_get_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_get_string (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, url), ret);
}

static void
swfdec_text_format_set_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_text_format_set_string (object,
      G_STRUCT_OFFSET (SwfdecTextFormat, url), argc, argv);
}

static void
swfdec_text_format_add_variable (SwfdecAsObject *object, const char *variable,
    SwfdecAsNative get, SwfdecAsNative set)
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

SWFDEC_AS_CONSTRUCTOR (110, 0, swfdec_text_format_construct, swfdec_text_format_get_type)
void
swfdec_text_format_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  guint i;

  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_TEXTFORMAT (object));

  if (!SWFDEC_PLAYER (cx)->text_format_properties_initialized) {
    SwfdecAsValue val;
    SwfdecAsObject *proto;

    swfdec_as_object_get_variable (object, SWFDEC_AS_STR___proto__, &val);
    g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
    proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_align,
	swfdec_text_format_get_align, swfdec_text_format_set_align);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_blockIndent,
	swfdec_text_format_get_blockIndent, swfdec_text_format_set_blockIndent);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_bold,
	swfdec_text_format_get_bold, swfdec_text_format_set_bold);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_bullet,
	swfdec_text_format_get_bullet, swfdec_text_format_set_bullet);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_color,
	swfdec_text_format_get_color, swfdec_text_format_set_color);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_display,
	swfdec_text_format_get_display, swfdec_text_format_set_display);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_font,
	swfdec_text_format_get_font, swfdec_text_format_set_font);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_indent,
	swfdec_text_format_get_indent, swfdec_text_format_set_indent);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_italic,
	swfdec_text_format_get_italic, swfdec_text_format_set_italic);
    if (cx->version >= 8) {
      swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_kerning,
	  swfdec_text_format_get_kerning, swfdec_text_format_set_kerning);
    }
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_leading,
	swfdec_text_format_get_leading, swfdec_text_format_set_leading);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_leftMargin,
	swfdec_text_format_get_leftMargin, swfdec_text_format_set_leftMargin);
    if (cx->version >= 8) {
      swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_letterSpacing,
	  swfdec_text_format_get_letterSpacing,
	  swfdec_text_format_set_letterSpacing);
    }
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_rightMargin,
	swfdec_text_format_get_rightMargin, swfdec_text_format_set_rightMargin);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_size,
	swfdec_text_format_get_size, swfdec_text_format_set_size);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_tabStops,
	swfdec_text_format_get_tabStops, swfdec_text_format_set_tabStops);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_target,
	swfdec_text_format_get_target, swfdec_text_format_set_target);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_underline,
	swfdec_text_format_get_underline, swfdec_text_format_set_underline);
    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_url,
	swfdec_text_format_get_url, swfdec_text_format_set_url);

    SWFDEC_PLAYER (cx)->text_format_properties_initialized = TRUE;
  }

  format = SWFDEC_TEXTFORMAT (object);
  format->blockIndent = NAN;
  format->bold = SWFDEC_TOGGLE_UNDEFINED;
  format->bullet = SWFDEC_TOGGLE_UNDEFINED;
  format->color = NAN;
  format->display = SWFDEC_TEXT_DISPLAY_BLOCK;
  format->indent = NAN;
  format->italic = SWFDEC_TOGGLE_UNDEFINED;
  format->kerning = SWFDEC_TOGGLE_UNDEFINED;
  format->leading = NAN;
  format->leftMargin = NAN;
  SWFDEC_AS_VALUE_SET_NULL (&format->letterSpacing);
  format->rightMargin = NAN;
  format->size = NAN;
  format->underline = SWFDEC_TOGGLE_UNDEFINED;

  i = 0;
  if (argc > i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_font, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_size, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_color, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_bold, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_italic, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_underline, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_url, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_target, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_align, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_leftMargin, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_rightMargin, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_indent, &argv[i]);
  if (argc > ++i)
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_leading, &argv[i]);
}
